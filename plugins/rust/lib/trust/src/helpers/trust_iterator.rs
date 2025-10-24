// Implements iteratos for TruST.
//

use event_utils::{EventId, EventInfo, EventType};
use execution_graph::ExecutionGraph;
use serde_derive::{Deserialize, Serialize};

#[derive(Clone, Copy, PartialEq, Debug, Serialize, Deserialize)]
pub enum TrustEventIterator {
    /// EventId of the current write it is reading from.
    ReadIterator(EventId),
    /// EventId of the write that comes before in co-order.
    WriteCOIterator(EventId),
    /// EventId of the `read` in backwards revisit and
    /// of the `write` that comes before in co-order, in this
    /// order.
    WriteBackwardsRevisitIterator(EventId, EventId),
}
use crate::TrustEventIterator::*;
/// We cant implement Iterator directly, because the `next` iterator
/// depends on the ExecutionGraph, and we dont want to have to save
/// the whole graph here. The iterators must be light because they
/// are saved in the recursion stack, and also in a file between
/// runs.
///
/// We try to make it in a similar format, by having only the
/// `next` and `init` methods public.
impl TrustEventIterator {
    #[allow(dead_code)]
    pub fn write_id_from_read_iterator(&self) -> EventId {
        match self {
            Self::ReadIterator(w) => *w,
            _ => panic!("Unexpected iterator type. Must be a ReadIterator"),
        }
    }
    #[allow(dead_code)]
    pub fn write_id_from_write_co_iterator(&self) -> EventId {
        match self {
            Self::WriteCOIterator(w) => *w,
            _ => panic!("Unexpected iterator type. Must be a WriteCOIterator"),
        }
    }
    #[allow(dead_code)]
    pub fn ids_from_write_backwards_revisit_iterator(&self) -> (EventId, EventId) {
        match self {
            Self::WriteBackwardsRevisitIterator(r, w) => (*r, *w),
            _ => panic!("Unexpected iterator type. Must be a WriteBackwardsRevisitIterator"),
        }
    }

    pub fn new(graph: &mut ExecutionGraph, tid: usize, ev_info: EventInfo) -> TrustEventIterator {
        match ev_info.event_type {
            EventType::Read => TrustEventIterator::init_iterator_of_read(graph, tid, ev_info),
            EventType::Write => TrustEventIterator::init_iterator_of_write(graph, tid, ev_info),
            _ => {
                panic!(
                    "Unexpected event type asking for iterators: {:?}",
                    ev_info.event_type
                )
            }
        }
    }

    /// Returns the next iterator or None if we are at the end.
    pub fn next(
        graph: &mut ExecutionGraph,
        tid: usize,
        ev_info: EventInfo,
        iter: TrustEventIterator,
    ) -> Option<TrustEventIterator> {
        match ev_info.event_type {
            EventType::Read => TrustEventIterator::next_iterator_of_read(graph, tid, ev_info, iter),
            EventType::Write => {
                TrustEventIterator::next_iterator_of_write(graph, tid, ev_info, iter)
            }
            _ => {
                panic!(
                    "Unexpected event type asking for iterators: {:?}",
                    ev_info.event_type
                )
            }
        }
    }

    /// Next write candidate for rf-edge.
    /// Returns `None` if all the candidates where explored.
    /// This works by iterating on `tid` in increasing order
    /// and `n` in decresing order.
    ///
    /// TODO [optimization]: iterate only on writes on the same
    /// address. **Careful with mixed-size access**!
    /// I think the best idea would be to have for each
    /// thread a list of writes/reads to each address, in the order
    /// they happen (that should be easy to maintain).
    /// Also if there is a write from current thread
    /// we dont try to read-from INIT.
    fn next_iterator_of_read(
        graph: &mut ExecutionGraph,
        tid: usize,
        ev_info: EventInfo,
        iter: TrustEventIterator,
    ) -> Option<TrustEventIterator> {
        match iter {
            ReadIterator(write_id) => {
                let init_ev = EventId::new(0, 0);
                if write_id == init_ev {
                    // INIT is the last event we try to read-from
                    return None;
                }
                let cur_tid = write_id.tid;
                let cur_n = write_id.n;
                let max_tid = graph.get_number_of_tasks();
                for ntid in cur_tid..=max_tid {
                    let init_n = if ntid == cur_tid {
                        cur_n
                    } else {
                        graph.get_number_of_events_in_task(ntid)
                    };
                    for n in (0..init_n).rev() {
                        let ev = EventId::new(ntid, n);
                        if graph.get_event_info(&ev).addr == ev_info.addr
                            && graph.get_event_info(&ev).event_type == EventType::Write
                            && graph.can_add_rf_edge(ev, tid, ev_info)
                        {
                            return Some(ReadIterator(ev));
                        }
                        if tid == ntid
                            && graph.get_event_info(&ev).addr == ev_info.addr
                            && graph.get_event_info(&ev).event_type == EventType::Write
                        {
                            // it can only read from latest write from the same thread.
                            break;
                        }
                    }
                }
                if graph.can_add_rf_edge(init_ev, tid, ev_info) {
                    return Some(ReadIterator(init_ev));
                }
                None
            }
            _ => panic!("Unexpected TrustEventIterator for read event {:?}", iter),
        }
    }

    /// Returns a pointer to the first event a
    /// read could be reading-from.
    /// Use this with `next_iterator_of_read` and you can get a list of
    /// all the possible `rf` edges.
    /// The order of events processed will be:
    /// from increasing order of thread_id and decreasing `n`, and lastly
    /// the initial state.
    /// TODO: Optimizations: For the current thread we only try to read
    /// from last write.
    fn init_iterator_of_read(
        graph: &mut ExecutionGraph,
        tid: usize,
        ev_info: EventInfo,
    ) -> TrustEventIterator {
        let max_tid = graph.get_number_of_tasks();
        for ntid in 1..=max_tid {
            let len = graph.get_number_of_events_in_task(ntid);
            for n in (0..len).rev() {
                let ev = EventId::new(ntid, n);
                if graph.get_event_info(&ev).addr == ev_info.addr
                    && graph.get_event_info(&ev).event_type == EventType::Write
                    && graph.can_add_rf_edge(ev, tid, ev_info)
                {
                    return ReadIterator(ev);
                }
            }
        }
        // if no other write, return INIT
        if !graph.can_add_rf_edge(EventId::new(0, 0), tid, ev_info) {
            graph.print_edges();
            graph.print_fr_edges();
            graph.print_trace();
        }
        assert!(graph.can_add_rf_edge(EventId::new(0, 0), tid, ev_info));
        ReadIterator(EventId::new(0, 0))
    }

    /// Initializes the Iterator for the write.
    /// We process in order of increasing thread_id, decreasing `n` first for the reads
    /// and with an inner loop for the writes.
    ///
    /// After all backwards revisit we attempt to set only the CO-edge.
    /// If the current id is (0, 1) this means this is the first event
    /// TODO [optimization]: Iterate only in reads/write to the right
    /// address (be aware of mixed-size accesses).
    fn init_iterator_of_write(
        graph: &mut ExecutionGraph,
        tid: usize,
        ev_info: EventInfo,
    ) -> TrustEventIterator {
        // first try backwards revisits:
        let read_id = TrustEventIterator::next_backwards_revisit_read_candidate(
            graph,
            tid,
            ev_info,
            EventId::new(0, 1),
        );
        if let Some(read_id) = read_id {
            // can backwards revisit
            let write_id = TrustEventIterator::next_backwards_revisit_write_candidate(
                graph,
                tid,
                ev_info,
                read_id,
                EventId::new(0, 1),
            );
            return WriteBackwardsRevisitIterator(read_id, write_id.unwrap());
        }
        // first CO-candidate
        let co = TrustEventIterator::next_co_candidate(graph, tid, ev_info, EventId::new(0, 1));
        WriteCOIterator(co.unwrap())
    }

    fn next_iterator_of_write(
        graph: &mut ExecutionGraph,
        tid: usize,
        ev_info: EventInfo,
        iter: TrustEventIterator,
    ) -> Option<TrustEventIterator> {
        match iter {
            WriteCOIterator(write_id) => {
                // no more backwards revisit
                if let Some(w) =
                    TrustEventIterator::next_co_candidate(graph, tid, ev_info, write_id)
                {
                    return Some(WriteCOIterator(w));
                }
                None
            }
            WriteBackwardsRevisitIterator(read_id, write_id) => {
                // try to get next write with this same read:
                let new_write_id = TrustEventIterator::next_backwards_revisit_write_candidate(
                    graph, tid, ev_info, read_id, write_id,
                );
                if let Some(new_write_id) = new_write_id {
                    return Some(WriteBackwardsRevisitIterator(read_id, new_write_id));
                }
                // check for other read candidates:
                let new_read_id = TrustEventIterator::next_backwards_revisit_read_candidate(
                    graph, tid, ev_info, read_id,
                );
                if new_read_id.is_none() {
                    // end of backwards revisit.
                    // add co edges:
                    if let Some(w) = TrustEventIterator::next_co_candidate(
                        graph,
                        tid,
                        ev_info,
                        EventId::new(0, 1),
                    ) {
                        return Some(WriteCOIterator(w));
                    }
                    return None;
                }
                let new_read_id = new_read_id.unwrap();
                let new_write_id = TrustEventIterator::next_backwards_revisit_write_candidate(
                    graph,
                    tid,
                    ev_info,
                    new_read_id,
                    EventId::new(0, 1),
                )
                .unwrap();
                Some(WriteBackwardsRevisitIterator(new_read_id, new_write_id))
            }
            _ => panic!("Unexpected Iterator Type for writes"),
        }
    }

    fn next_backwards_revisit_read_candidate(
        graph: &mut ExecutionGraph,
        tid: usize,
        ev_info: EventInfo,
        read_id: EventId,
    ) -> Option<EventId> {
        let cur_tid = read_id.tid;
        let cur_n = read_id.n;
        let max_tid = graph.get_number_of_tasks();
        for ntid in cur_tid..=max_tid {
            if ntid == 0 {
                continue;
            }
            let init_n = if ntid == cur_tid {
                cur_n
            } else {
                graph.get_number_of_events_in_task(ntid)
            };
            for n in (0..init_n).rev() {
                if tid == ntid {
                    break; // cant backwards revisit in own thread
                }
                let ev = EventId::new(ntid, n);
                // TODO: here we could directly iterate only on Reads to
                // the right address.
                if graph.get_event_info(&ev).addr == ev_info.addr
                    && graph.get_event_info(&ev).event_type == EventType::Read
                    && graph.can_backwards_revisit(&ev, tid, ev_info)
                {
                    if !ev_info.is_rmw {
                        return Some(ev);
                    }
                    // for the case of rmw, it may not have a co-edge candidate, so we
                    // must check it here:
                    let n_rmw = graph.get_number_of_events_in_task(tid);
                    assert!(n_rmw > 0);
                    let r_rmw = EventId::new(tid, n_rmw - 1);
                    let w_prv = graph.get_rf_mapping(&r_rmw);
                    if graph.can_add_co_order_in_backwards_revist(w_prv, ev, tid, ev_info) {
                        return Some(ev);
                    }
                }
            }
        }
        None
    }

    fn next_backwards_revisit_write_candidate(
        graph: &mut ExecutionGraph,
        tid: usize,
        ev_info: EventInfo,
        read_id: EventId,
        write_id: EventId,
    ) -> Option<EventId> {
        let init_ev = EventId::new(0, 0);
        if write_id == init_ev {
            return None; // done
        }
        let cur_tid = write_id.tid;
        let cur_n = write_id.n;
        let max_tid = graph.get_number_of_tasks();

        for ntid in cur_tid..=max_tid {
            if ntid == 0 {
                continue;
            }
            let init_n = if ntid == cur_tid {
                cur_n
            } else {
                graph.get_number_of_events_in_task(ntid)
            };
            for n in (0..init_n).rev() {
                let ev = EventId::new(ntid, n);
                // TODO: here we could directly iterate only on Writes to
                // the right address.
                if graph.get_event_info(&ev).addr == ev_info.addr
                    && graph.get_event_info(&ev).event_type == EventType::Write
                    && graph.can_add_co_order_in_backwards_revist(ev, read_id, tid, ev_info)
                {
                    return Some(ev);
                }
            }
        }
        // try to go to Init
        if graph.can_add_co_order_in_backwards_revist(init_ev, read_id, tid, ev_info) {
            return Some(init_ev);
        }
        None
    }

    /// Returns the next possible co-edge. We iterate edges from
    /// lowest to highest `cur_tid` and highest to lowest event order (`cur_n`).
    /// The current `(cur_tid, cur_n)` is encoded in the argument `write_id`.
    ///
    /// The last tried edge was with `write_id` coming right before
    /// the new event, and there is no backwards revisit here.
    fn next_co_candidate(
        graph: &mut ExecutionGraph,
        tid: usize,
        ev_info: EventInfo,
        write_id: EventId,
    ) -> Option<EventId> {
        let init_ev = EventId::new(0, 0);
        if write_id == init_ev {
            return None; // done
        }
        let cur_tid = write_id.tid;
        let cur_n = write_id.n;
        let max_tid = graph.get_number_of_tasks();
        for ntid in cur_tid..=max_tid {
            if ntid == 0 {
                continue;
            }
            if ntid == tid && ntid == cur_tid {
                // We already considered a write from the same thread
                // We can safely assume that all the other writes will
                // not be consistent because they break SC-per-loc.
                continue;
            }
            let init_n = if ntid == cur_tid {
                cur_n
            } else {
                graph.get_number_of_events_in_task(ntid)
            };
            for n in (0..init_n).rev() {
                let ev = EventId::new(ntid, n);
                // TODO: here we could directly iterate only on Writes
                // to the right address.

                if graph.get_event_info(&ev).addr == ev_info.addr
                    && graph.get_event_info(&ev).event_type == EventType::Write
                    && graph.can_add_co_order(ev, tid, ev_info)
                {
                    return Some(ev);
                }
                if ntid == tid
                    && graph.get_event_info(&ev).addr == ev_info.addr
                    && graph.get_event_info(&ev).event_type == EventType::Write
                {
                    // For the same thread we only must consider one such write.
                    break;
                }
            }
        }
        // try to go to Init
        if graph.can_add_co_order(init_ev, tid, ev_info) {
            return Some(init_ev);
        }
        None
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use event_utils::EventValue;
    use lotto::engine::handler::{AddrSize, Address};
    use std::num::NonZeroUsize;

    #[allow(dead_code)]
    fn mocked_read(addr: Address) -> EventInfo {
        EventInfo::new(
            0,
            Some(addr),
            Some(AddrSize::new(4)),
            EventType::Read,
            false,
            /* is_rmw */ false,
            EventValue::NoneValue,
        )
    }

    #[allow(dead_code)]
    fn mocked_write(addr: Address) -> EventInfo {
        EventInfo::new(
            0,
            Some(addr),
            Some(AddrSize::new(4)),
            EventType::Write,
            false,
            /* is_rmw */ false,
            EventValue::NoneValue,
        )
    }

    #[test]
    fn test_read_iterator() {
        // Anottated on the side of the write is the co-order.
        // Note that Rx cant read from thread 1, because Wx (2) came after
        // Wx (1), so there would be a fr edge from Rx to Wx (2), causing a loop.
        //        INIT
        //      Wx (1) |  Wx(3) | Wx (2)  |  Wx (4)
        //             |        | Rx      |  Wy
        let mut g = ExecutionGraph::new();
        let init_ev = EventId::new(0, 0);
        let addr1 = Address::new(NonZeroUsize::new(1).unwrap());
        let addr2 = Address::new(NonZeroUsize::new(2).unwrap());
        let ev_info1 = mocked_write(addr1);
        let w1 = g.add_new_write_event(1, ev_info1, init_ev);

        let ev_info2 = mocked_write(addr1);
        let w3 = g.add_new_write_event(3, ev_info2, w1);

        let ev_info3 = mocked_write(addr1);
        let w2 = g.add_new_write_event(2, ev_info3, w3);

        let ev_info4 = mocked_write(addr1);
        let w4x = g.add_new_write_event(4, ev_info4, w2);

        let ev_info5 = mocked_write(addr2); // useless address
        let _w4y = g.add_new_write_event(4, ev_info5, init_ev);

        // now the read:
        let ev_info_read = mocked_read(addr1);
        let tid = 3;

        let iter = TrustEventIterator::new(&mut g, tid, ev_info_read);
        assert!(iter == ReadIterator(w2));
        let iter = TrustEventIterator::next(&mut g, tid, ev_info_read, iter).unwrap();
        assert!(iter == ReadIterator(w3));
        let iter = TrustEventIterator::next(&mut g, tid, ev_info_read, iter).unwrap();
        assert!(iter == ReadIterator(w4x));
        let iter = TrustEventIterator::next(&mut g, tid, ev_info_read, iter);
        // Init should not be possible!
        assert!(iter.is_none());
        // OK :D
        //        INIT
        //      Wx (1) |  Wy (2)
        //      Ry (3) |  Rx (4)
        let mut g = ExecutionGraph::new();

        let ev_info1 = mocked_write(addr1);
        let w_x_1 = g.add_new_write_event(1, ev_info1, init_ev);

        let ev_info2 = mocked_write(addr2);
        let w_y_2 = g.add_new_write_event(2, ev_info2, init_ev);

        let ev_info3 = mocked_read(addr2);
        let _r_y_1 = g.add_new_read_event(1, ev_info3, w_y_2);

        // now the read:
        let ev_info_read = mocked_read(addr1);
        let tid = 2;

        let iter = TrustEventIterator::new(&mut g, tid, ev_info_read);
        assert!(iter == ReadIterator(w_x_1));
        let iter = TrustEventIterator::next(&mut g, tid, ev_info_read, iter).unwrap();
        assert!(iter == ReadIterator(init_ev));
        let iter = TrustEventIterator::next(&mut g, tid, ev_info_read, iter);
        assert!(iter.is_none());
        // OK test 2 :D
    }

    #[test]
    fn test_write_iterator() {
        // must be able to first have all the possible backwards revisits,
        // for each revisit should have the correct co-edge candidates,
        // and finally should return a list of co-edge candidates.

        //                   INIT
        //        (1) Rx | (3) Rx | (4) Rx(rf from (INIT))
        //        (2) Wx |        | (5) Wx
        //
        // only possible candidate is (1) and (3). (3) also needs to be maximally added
        // so (3) reads-from (2)
        // cant read from (4) due to porf dependencie.
        //
        let mut g = ExecutionGraph::new();
        let init_ev = EventId::new(0, 0);
        let addr1 = Address::new(NonZeroUsize::new(1).unwrap());
        let _addr2 = Address::new(NonZeroUsize::new(2).unwrap());

        let ev_info1 = mocked_read(addr1);
        let r1 = g.add_new_read_event(1, ev_info1, init_ev);

        let ev_info2 = mocked_write(addr1);
        let w1 = g.add_new_write_event(1, ev_info2, init_ev);

        let ev_info3 = mocked_read(addr1);
        let r2 = g.add_new_read_event(2, ev_info3, w1);

        let ev_info4 = mocked_read(addr1);
        let _r3 = g.add_new_read_event(3, ev_info4, init_ev);

        // new event:
        let tid = 3;
        let ev_info = mocked_write(addr1);
        let iter = TrustEventIterator::new(&mut g, tid, ev_info);
        assert!(iter == WriteBackwardsRevisitIterator(r1, init_ev));
        let iter = TrustEventIterator::next(&mut g, tid, ev_info, iter).unwrap();
        dbg!(iter);
        assert!(iter == WriteBackwardsRevisitIterator(r2, w1));
        let iter = TrustEventIterator::next(&mut g, tid, ev_info, iter).unwrap();
        assert!(iter == WriteBackwardsRevisitIterator(r2, init_ev));

        let iter = TrustEventIterator::next(&mut g, tid, ev_info, iter).unwrap();
        // no more backwards revisit
        assert!(iter == WriteCOIterator(w1));
        let iter = TrustEventIterator::next(&mut g, tid, ev_info, iter).unwrap();
        assert!(iter == WriteCOIterator(init_ev));
        let iter = TrustEventIterator::next(&mut g, tid, ev_info, iter);
        assert!(iter.is_none());

        // SECOND TEST
        // (1) Wx | | (3) Wy
        // (2) Ry | |
        let addr_x = Address::new(NonZeroUsize::new(3).unwrap());
        let addr_y = Address::new(NonZeroUsize::new(4).unwrap());
        let mut g = ExecutionGraph::new();
        let ev_info1 = mocked_write(addr_x);
        let _w1 = g.add_new_write_event(1, ev_info1, init_ev);

        let ev_info2 = mocked_read(addr_y);
        let r1 = g.add_new_read_event(1, ev_info2, init_ev);

        // new event:
        let tid = 3;
        let ev_info = mocked_write(addr_y);
        let iter = TrustEventIterator::new(&mut g, tid, ev_info);
        assert!(iter == WriteBackwardsRevisitIterator(r1, init_ev));
        let iter = TrustEventIterator::next(&mut g, tid, ev_info, iter).unwrap();
        assert!(iter == WriteCOIterator(init_ev));
        let iter = TrustEventIterator::next(&mut g, tid, ev_info, iter);
        assert!(iter.is_none());
        // OK test 2
        // TEST 3
        let mut g = ExecutionGraph::new();
        // test RR + W
        let rx_info = mocked_read(addr_x);
        let r1 = g.add_new_read_event(1, rx_info, init_ev);
        let r2 = g.add_new_read_event(1, rx_info, init_ev);

        let tid = 2;
        let ev_info = mocked_write(addr_x);
        let iter = TrustEventIterator::new(&mut g, tid, ev_info);
        assert!(iter == WriteBackwardsRevisitIterator(r2, init_ev));
        let iter = TrustEventIterator::next(&mut g, tid, ev_info, iter).unwrap();
        assert!(iter == WriteBackwardsRevisitIterator(r1, init_ev));
        let iter = TrustEventIterator::next(&mut g, tid, ev_info, iter).unwrap();
        assert!(iter == WriteCOIterator(init_ev));
        let iter = TrustEventIterator::next(&mut g, tid, ev_info, iter);
        assert!(iter.is_none());
        // OK test 3
    }
}
