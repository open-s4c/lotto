// Implements the Truly Stateless DPOR algorithm from https://plv.mpi-sws.org/genmc/popl2022-trust.pdf

#![allow(clippy::doc_lazy_continuation)]
#![allow(clippy::ptr_arg)]

//!
//! Given its current state and the next event, it should decide what to do next.
//! The allowed results are:
//! - Indicate that every execution Graph was visited.
//! - Exit with an error.
//! - Add the new event.
//! - Start again from another trace. This is what happens for most case, such as
//! a backwards-revisit, a non-maximal foward revisit, and a non-maximal co-edge add
//! The main disadvantage of having TruSt inside of lotto is this huge overhead
//! due to restarting...
//! It is also responsible for updating it's own State
//!
//! TODO: Ensure we find a good order to do the foward revisits:
//! - iterate over each (tid,n) and pick only the latest write to the
//! required address. -> isnt it any write, with the exception we read from
//! our own thread? -> If our thread already wrote, we also cant read from INIT!
//!
//! - first operation must be in the co-maximal write
//!
//! For co-edges we first try to add an edge from the co-maximal?
use crate::helpers::state::AddressInfo;
use crate::helpers::trust_iterator::TrustEventIterator::*;
use crate::helpers::virtual_address_mapper::VirtualAddressMapper;
use crate::helpers::{state::TrustState, trust_iterator::TrustEventIterator};
use core::panic;
use event_utils::{EventId, EventInfo, EventType, EventValue};
use execution_graph::ExecutionGraph;
use helpers::state::TrustStackFrame;
use lotto::collections::{FxHashMap, FxHashSet};
use lotto::engine::handler::AddrSize;
use lotto::engine::handler::{Address, TaskId, ValuesTypes};
use lotto::log::{debug, trace};
use serde_derive::Deserialize;
use serde_derive::Serialize;
pub mod helpers;
#[cfg(test)]
mod trust_tests;

/// Implements the algorithm from https://plv.mpi-sws.org/genmc/popl2022-trust.pdf
/// The actual handler is implemented separately to make it easier to test, debug
/// and improve code.
///
/// # Overview of the algorithm
///
/// In the trust paper [https://plv.mpi-sws.org/genmc/popl2022-trust.pdf], they build up on some
/// ideas of the first GenMC (generic model checker) paper: https://plv.mpi-sws.org/genmc/full-paper.pdf.
/// In this first paper, they create a Optimal Dynamic Partial Order Reduction, that is, a model
/// checker that creates every possible interleaving of threads up to some equivalence class,
/// with a single execution for each class. The main idea is to represent an execution by an Events Graph,
/// and add events one by one. If the event is a read, we must consider each **rf** (reads-from) edge
/// that it could be reading from (this is called a foward revisit). If the event is a write, we must
/// consider each **co** (coherence order) edge, and also each rf edge that we may have missed
/// (this is called a backwards revisit).
///
/// The novel difference is that, while in the first paper we always check for backward revisits and
/// have to keep a set of explored events, now we consider backwards revisit if, and only if, the
/// execution graph is a "maximal extension of w to r".
///
/// The maximal extension is a propertie that holds for exactly one consistent execution graph amidst
/// all that could potentially create a backward revisit (i.e. consident executions that
/// don't create porf cycles).
///
/// In this way, it is possible to explore every equivalence class of interleavings with **polynomial
/// memory requirements**, in a depth-first manner.
///
/// ## Attention point in backwards-revisit
///
/// This is the hardest part to implement, because when backwards-revisiting we may need to
/// remove some events. As the algorithm is done in a depth-first manner, we must also be able
/// to **undo** a backwards revisit.
///
/// The way we are able to remove event and also force the read to rf a particular write is:
///
/// - We find a trace that follows the partial-order induced by
///   the (po U co U rf U fr)+ edges. We call this **reconstruction trace**;
/// - We shutdown the current lotto execution;
/// - We add events following the **reconstruction trace**.
///
/// For undoing a backwards revisit, we still need to find a way to optimize it. For now,
/// we re-do all the operations in the stack, which is far from ideal.
///
/// ### Some notation
///
/// In the paper they use two different total orders: <_{next} and <_{G}.
///
/// <_{next}: keeps the order in which each event has priority in being added to the graph.
///
/// <_{G}: Keeps a total order of which event was added before another in the graph. Note
/// that we delete events in case of backwards-revisits, therefore these orders not always
/// agree.
///
/// ### Dealing with RMW
///
/// **Weak CAS are ignored!**
///
/// To preserve the architecture, the Read-Modify-Write's must consist of
/// two separate events, with the write immediatly following the read in <_{next}
/// order.
///
/// When adding the read, we check the values to see if the CAS will succed. If not,
/// it is a normal read. Else, we must set `is_rwm` to true and see the next TruST
/// action.
///
/// In the function `next_thread_to_run` (which comes from the real handler/is mocked),
/// in case we are reconstructing and get to a point where we add a `is_rwm=true` read
/// and there is no exclusive write after, we will:
///
/// 1) Assert that this is the **last** event in the trace;
///
/// 2) Add the write part of the RMW as the next event;
///
/// ### Serialization/Deserialization
///
/// For quick implementation of serialization we use rust serde_json
/// library. This allows us to use `derive` notation to quickly serialize
/// an object.
///
/// The TruST object is serialized into a JSON string, which can be saved in
/// a file with memory mapping for fast access.
///
/// The serialized objects are:
///
/// - `state`: Here we dont serialize the whole state only:
///     - `stack`
///     - `rebuild_trace`
///     - `execution_graph`: we only serialize the `cur_id_count`,
///       which keeps the number of already added nodes. All the
///       other information is cleaned and must be initialized after
///       deserializing.
/// - `cached_address_info`: We keep a mapping of virtual address and
///   the initial value. We must initialize it properly after deserializing.
/// - `total_graphs`
/// - `virtual_address_mapper`:  We keep only the number of created virtual
/// mappings, because the other information can change in the next run.
///
/// To serialize use `self.serialize()` and to deserialize please use
/// `Trust::deserialize_and_init(str)`.
///
#[derive(Serialize, Deserialize)]
pub struct TruST {
    /// state of the algorithm. Needs to be serialized and saved between runs.
    state: TrustState,
    // The following components are for internal use and dont need to be
    // serialized between runs.
    /// If there is no writes yet there should be no entry.
    #[serde(skip)]
    last_write_to_addr: FxHashMap<Address, EventId>,
    /// In case a successful RMW read is just processed, this
    /// will be a Some(tid, addr). Otherwise, it is None.
    /// As soon as we process the write, we must make sure to
    /// change this back to None.
    #[serde(skip)]
    expected_rmw_write: Option<(TaskId, Address, AddrSize, EventValue)>,
    cached_address_info: FxHashMap<Address, AddressInfo>,
    /// In case of Some, this marks a thread that had a CAS
    /// that now is not successful. We must skip all of its later events
    /// during the rebuild.
    #[serde(skip)]
    skip_failed_cas_tid_in_rebuild: Option<TaskId>,
    #[serde(skip)]
    failed_cas_tid: Option<TaskId>,
    total_graphs: u64,
    /// Helper to switch into real addresses, and a abstraction used in
    /// the TruST algorithm. This is necessary because a address can change
    /// in between runs.
    virtual_address_mapper: VirtualAddressMapper,
    /// `true` if TruST is not integrated with lotto, and used
    /// only for testing purposes. We need this to know if we should
    /// mock the initial values of each address and for potential debug.
    test_mode: bool,
}

#[derive(Debug, PartialEq)]
pub enum TrustAction {
    /// Marks that we add the event.
    Add,
    /// Marks that we must restart following a
    /// trace in the state. We find a trace that implicitely
    /// will generate the execution graph by getting a topological sort.
    Rebuild,
    /// Marks that we visited every possible execution graph and the
    /// algorithm must terminate.
    End,
}

impl Default for TruST {
    fn default() -> Self {
        Self::new()
    }
}

impl TruST {
    /// Handles the addition of a new event from thread `tid`, with the
    /// information at `ev_info`. Modifies the state of the handler and
    /// returns the next Action.
    pub fn handle_new_event(&mut self, tid: usize, ev_info: EventInfo) -> TrustAction {
        if self.expected_rmw_write.is_some() {
            self.assert_equals_expected_rmw_write(&tid, &ev_info);
            self.expected_rmw_write = None; // used
        }
        if self.is_rebuilding() {
            return self.add_event_rebuild(tid, ev_info);
        }
        let real_address = ev_info.addr;
        let mut ev_info = ev_info;
        // Not reconstruction
        // Update mapping of real address to virtual, if needed:
        if ev_info.addr.is_some() {
            ev_info.addr = Some(
                self.virtual_address_mapper
                    .get_virtual_address_or_create(&real_address.unwrap()),
            );
        }

        match ev_info.event_type {
            EventType::Error => {
                panic!("Error found on the graph!")
            }
            EventType::Init => {
                panic!("Unexpected event type")
            }
            EventType::Read => {
                let mut new_ev = self.new_read_stack_item(tid, ev_info);

                let mut ev_info = ev_info;
                if self.test_mode && EventInfo::is_cas(&ev_info) {
                    self.update_rmw_state(&mut ev_info, new_ev.iterator);
                    // Also update on the stack
                    new_ev.event_info = ev_info;
                }

                self.state.stack.push(new_ev);
                let action = self.handle_read(tid, ev_info);
                if action == TrustAction::Add && ev_info.is_rmw {
                    // next must be Write
                    let val = EventValue::WriteValue(EventValue::get_rmw_write_value(ev_info.val));
                    trace!("Expecting now a RMW write for {:?}", tid);
                    self.signal_expecting_rmw_write(
                        TaskId::new(tid as u64),
                        real_address.unwrap(),
                        ev_info.addr_length.unwrap(),
                        val,
                    );
                }
                action
            }
            EventType::Write => {
                let new_ev = self.new_write_stack_item(tid, ev_info);
                self.state.stack.push(new_ev);
                self.handle_write(tid, ev_info)
            }
            EventType::None => {
                // ok, just ignore.
                // Note that we purposly dont add this to the stack
                // to save space.
                // TODO: Maybe we need to add "CAT_TASK_CREATE" to the stack though,
                // because in the handler this may be needed for the `reconstruction_trace`.
                TrustAction::Add
            }
            EventType::End => {
                // Finished.
                // Backtrack to next event on the stack or signal that
                // all the graphs were visited.
                self.handle_full_execution_graph()
            }
        }
    }

    fn add_event_rebuild(&mut self, tid: usize, ev_info: EventInfo) -> TrustAction {
        if ev_info.event_type == EventType::None {
            // this was not added to the trace so we just return.
            return TrustAction::Add;
        }
        let mut ev_info = ev_info;
        self.state.rebuild_trace_ptr += 1;
        let (expec_tid, expec_ev_info) = self.state.rebuild_trace[self.state.rebuild_trace_ptr];
        if ev_info.addr.is_some() {
            ev_info.addr = Some(self.virtual_address_mapper.add_mapping_in_rebuild(
                /* real_address */ ev_info.addr.unwrap(),
                /* virtual_address */ expec_ev_info.addr.unwrap(),
            ));
        }

        if self.test_mode {
            // For tests we must update `is_rmw` status to match the current that
            // would be passed in a real cenario.
            self.update_rmw_status_for_tests_in_rebuild(&mut ev_info);
        }

        // Sanity checks
        self.assert_event_equals_expected_in_rebuild(&tid, &ev_info, &expec_tid, &expec_ev_info);

        let old_insertion_order = expec_ev_info.insertion_order;
        self.add_new_event_in_reconstruction_mode(tid, ev_info, old_insertion_order);
        if self.state.rebuild_trace_ptr + 1 >= self.state.rebuild_trace.len() {
            // this was the last event in reconstruction mode, so we
            // update our state.
            self.mark_end_of_reconstruction();
            return TrustAction::Add;
        }

        // Lotto applies both parts of a RMW at once, while we
        // try to treat it as two separate events. Because of this,
        // some events of the trace can be missing, but they can be skipped.
        self.skip_failed_cas_in_rebuild(tid, &ev_info, &expec_ev_info);

        trace!("Adding event in trust rebuild {:?} {:?}", tid, ev_info);
        TrustAction::Add
    }

    /// Handles the case where we are adding a read event.
    /// We must try to add every consistent rf edge. In case
    /// We are not reading from a maximal write, we must Rebuild,
    /// otherwise we add it to the graph and return TrustAction::Add.
    fn handle_read(&mut self, tid: usize, ev_info: EventInfo) -> TrustAction {
        let iterator = self.state.stack.last().unwrap().iterator;
        let write_id = match iterator {
            ReadIterator(write_id) => write_id,
            _ => panic!("Unexpected iterator for a read"),
        };
        let ev = self.state.graph.add_new_read_event(tid, ev_info, write_id);
        let len = self.state.stack.len();
        self.state
            .stack
            .get_mut(len - 1)
            .unwrap()
            .event_info
            .insertion_order = self.state.graph.get_insertion_order(&ev);
        if self.state.graph.is_maximal_write(&write_id) {
            // This is a comaximal write, so we just add.
            return TrustAction::Add;
        }
        // Get the new trace and rebuild.
        self.prepare_state_for_rebuild();
        TrustAction::Rebuild
    }

    /// Handles the addition of a new write event. This will test every
    /// possible backwards revisit, as well as every possible co-edge order.
    /// If we are NOT backwards revisiting AND we are co-maximal, we Add, else
    /// we need to Rebuild. Yes, this adds quite a bit of overhead...
    fn handle_write(&mut self, tid: usize, ev_info: EventInfo) -> TrustAction {
        let iterator = self.state.stack.last().unwrap().iterator;
        match iterator {
            WriteBackwardsRevisitIterator(read_id, write_id) => {
                // Backwards revisit
                trace!("Backwards revisiting!");
                let ev = self
                    .state
                    .graph
                    .perform_backwards_revist(read_id, write_id, tid, ev_info);
                let len = self.state.stack.len();
                self.state
                    .stack
                    .get_mut(len - 1)
                    .unwrap()
                    .event_info
                    .insertion_order = self.state.graph.get_insertion_order(&ev);
                self.prepare_state_for_rebuild();
                TrustAction::Rebuild
            }
            WriteCOIterator(write_id) => {
                // No backwards revisit. Must still check the co-edges.
                let ev = self.state.graph.add_new_write_event(tid, ev_info, write_id);
                let len = self.state.stack.len();
                self.state
                    .stack
                    .get_mut(len - 1)
                    .unwrap()
                    .event_info
                    .insertion_order = self.state.graph.get_insertion_order(&ev);
                if self.state.graph.is_maximal_write(&ev) {
                    // If we are a co-maximal write just add.
                    return TrustAction::Add;
                }
                // Else start again following a given trace.
                self.prepare_state_for_rebuild();
                TrustAction::Rebuild
            }
            _ => panic!("Unexpected Iterator for a write"),
        }
    }

    /// Sets the necessary variables for a rebuild.
    /// **Deletes** the whole execution graph. This is useful for the
    /// mocked tests. In the actual handler, we will shutdown the lotto run,
    /// so it doesnt matter.
    fn prepare_state_for_rebuild(&mut self) {
        assert!(self.state.rebuild_trace.is_empty());
        self.state.rebuild_trace = self.state.graph.trace_from_graph();
        self.state.rebuild_trace_ptr = 0;
        self.last_write_to_addr.clear();
        let cur_id_count = self.state.graph.get_id_count();
        // Delete graph. We will need to re-start the algorithm all over again, so
        // this doesnt seem to bad.
        self.state.graph.print_trace();
        self.state.graph = ExecutionGraph::new();
        // Important: we keep the previous 'insertion order count' to ensure all the
        // events to come have a higher id, and it doesnt repeat.
        self.state.graph.set_id_count(cur_id_count);
        self.expected_rmw_write = None;
        self.failed_cas_tid = None;
        self.skip_failed_cas_tid_in_rebuild = None;

        if self.test_mode {
            // update initial values
            for (addr, addr_info) in &self.cached_address_info {
                self.state
                    .graph
                    .update_init_value_if_not_set(*addr, addr_info.init_value);
            }
        }
    }

    pub fn is_rebuilding(&self) -> bool {
        !self.state.rebuild_trace.is_empty()
    }
    pub fn get_next_trace_event(&self) -> (TaskId, EventInfo) {
        self.state.rebuild_trace[self.state.rebuild_trace_ptr + 1]
    }

    /// Only use for tests and debug
    pub fn get_reconstruction_trace(&self) -> Vec<(TaskId, EventInfo)> {
        self.state.rebuild_trace.clone()
    }

    /// Cleans the trace vector and pointer.
    /// Looks if there is a read part of a RMW without the write.
    /// If so, we are expecting the write next!
    fn mark_end_of_reconstruction(&mut self) {
        debug!("End of reconstruction");
        if let Some((tid, addr, lenght, val)) = self.state.graph.get_unmatched_rmw() {
            let real_addr = self.virtual_address_mapper.get_real_address(&addr);
            self.signal_expecting_rmw_write(tid, real_addr, lenght, val);
        }
        self.state.rebuild_trace.clear();
        self.state.rebuild_trace_ptr = 0;
    }

    /// Skip all the consecutive events of `tid` in rebuild.
    pub fn skip_tid_in_rebuild(&mut self, tid: TaskId) {
        while self.state.rebuild_trace_ptr + 1 < self.state.rebuild_trace.len() {
            let (cur_tid, _ev_info) = self.state.rebuild_trace[self.state.rebuild_trace_ptr + 1];
            if tid == cur_tid {
                self.state.rebuild_trace_ptr += 1;
            } else {
                break;
            }
        }
        if self.state.rebuild_trace_ptr + 1 >= self.state.rebuild_trace.len() {
            // this was the last event in reconstruction mode, so we
            // update our state.
            self.mark_end_of_reconstruction();
        }
    }

    /// Lotto applies both parts of a RMW at once, while we
    /// try to treat it as two separate events. Because of this,
    /// some events of the trace can be missing, but they can be skipped.
    ///
    /// This function will take care of the logic of this special cenario.
    pub fn skip_failed_cas_in_rebuild(
        &mut self,
        tid: usize,
        ev_info: &EventInfo,
        expec_ev_info: &EventInfo,
    ) {
        let tid = TaskId::new(tid as u64);
        if !ev_info.is_rmw && expec_ev_info.is_rmw {
            // Now the read is reading from another write, so it can stop to be a RMW.
            // This happens because in Lotto the CAS is a single operation, regardless
            // of how we try to break it into two. So if we have something like
            // R_CAS(x) || R_CAS(x)
            // W_CAS(x) || [To be added W_CAS(x)]
            // R(y)     ||
            // We will add temporarly W_CAS(x) for the sake of the algorithm, but
            // we will skip all other events of this thread, because they will be
            // deleted as soon as we finish the rebuild.
            assert!(ev_info.event_type == EventType::Read);
            // There should be at most one such thread that can be skipped.
            self.failed_cas_tid = Some(tid);
        }
        if let Some(failed_tid) = self.failed_cas_tid {
            if tid == failed_tid && ev_info.event_type == EventType::Write {
                assert!(self.skip_failed_cas_tid_in_rebuild.is_none());
                assert!(ev_info.is_rmw); // We must wait the exclusive write part
                                         // before skipping all other events.
                self.skip_failed_cas_tid_in_rebuild = self.failed_cas_tid;
            }
        }
        if let Some(tid) = self.skip_failed_cas_tid_in_rebuild {
            debug!(
                "Marking to skip all events of thread {:?} during the rebuild",
                tid
            );
            self.skip_tid_in_rebuild(tid);
        }
    }

    /// Handles the case when there are no more threads to run.
    ///
    /// This will keep popping the stack until it finds a event that has
    /// new things to explore.
    ///
    /// When we pop from the stack, we must get the old Execution Graph.
    /// Currently this is done in a poor non-optimized manner :(.
    ///
    /// Calls the functions `handle_read` and `handle_write`.
    ///
    /// Always returns one of the Actions: `Rebuld` or `End`.
    fn handle_full_execution_graph(&mut self) -> TrustAction {
        debug!("Finished another graph. Printing its trace:");
        self.state.graph.print_trace();
        self.state.print_graph();
        self.sanity_check_stack_graph();
        self.sanity_check_graph();
        self.total_graphs += 1;

        while let Some(mut top) = self.state.stack.pop() {
            let tid = top.tid;
            let ev_info = top.event_info;
            trace!(
                "Looking at this event from the stack in Trust {:?}, {:?}, iterator: {:?}",
                tid,
                ev_info,
                top.iterator
            );
            match top.event_info.event_type {
                EventType::Read => {
                    self.state.graph.remove_last_event_of_thread(tid);

                    let mut ev_info = ev_info;
                    let nxt_iter =
                        TrustEventIterator::next(&mut self.state.graph, tid, ev_info, top.iterator);

                    if let Some(nxt_iter) = nxt_iter {
                        if EventInfo::is_cas(&ev_info) {
                            self.update_rmw_state(&mut ev_info, nxt_iter);
                        }
                        top.iterator = nxt_iter;
                        top.event_info = ev_info;
                        self.state.stack.push(top);
                        let size = self.state.stack.len();
                        let action = self.handle_read(tid, ev_info);
                        let cur_insertion_order = top.event_info.insertion_order;
                        if action == TrustAction::Add {
                            let ev = self.state.graph.last_event_of_thread(tid);
                            // When we remove and try to update the rf edge, we should also update
                            // the insertion_order.
                            self.update_insertion_order(size - 1, ev, cur_insertion_order);
                            let new_insertion_order =
                                self.state.stack[size - 1].event_info.insertion_order;
                            assert!(cur_insertion_order == new_insertion_order);
                            self.prepare_state_for_rebuild(); // force rebuild!
                        }
                        return TrustAction::Rebuild;
                    } else {
                        // No next element. For reads we automatically rebuilt the
                        // graph, because we are already deleted the event and it
                        // had no edges.
                    }
                }
                EventType::Write => {
                    match top.iterator {
                        WriteBackwardsRevisitIterator(_, _) => {
                            // Last event did a backwards revisit
                            // we can't simply delete it because then the read would have no edges.
                            // We also cant save the previous write R was "reading-from", because
                            // it may have being deleted.
                            // In this case, we will reconstruct the graph from the stack right
                            // away... Other options are: saving the old graph in the stack;
                            // In the paper they can get it with a function `prev(G)`, but it
                            // uses the function `next_thread_to_run(G, Program)`, which we dont
                            // have access.
                            self.state.graph = self.rebuild_graph_from_stack();
                            //    TODO: Maybe there is a order we
                            //        could add the writes so we dont need to worry about re-adding the
                            //        events in case of a backwards revisit...
                            //        How to do that do?
                            //        In case we are at the same backwards revisit and only change the co-edge
                            //        We dont need to rebuild, so we can try to figure out it if is the first
                            //        co-edge of this revisit.
                            //        if we change the read_id, then I see no easier option than
                            //        rebuilding it all.
                            // Quote from a discussion regarding this:
                            // '''The order should be that you revisit according to the event insertion order,
                            // and you revisit the most recently inserted one first.
                            // But that may not be helpful at all for Trust, even if it worked for the original genmc'''
                        }
                        WriteCOIterator(_) => {
                            // no backwards-revisit. We can remove it because there are no rf edges
                            // and the co-edges are implicitely set.
                            self.state.graph.remove_last_event_of_thread(tid);
                        }
                        _ => panic!("Unexpected iterator for a Write"),
                    }
                    let nxt_iter =
                        TrustEventIterator::next(&mut self.state.graph, tid, ev_info, top.iterator);

                    if let Some(nxt_iter) = nxt_iter {
                        top.iterator = nxt_iter;
                        let cur_insertion_order = top.event_info.insertion_order;
                        self.state.stack.push(top);
                        let size = self.state.stack.len();
                        let action = self.handle_write(tid, ev_info);
                        assert!(self.state.stack.len() == size);
                        if action == TrustAction::Add {
                            let ev = self.state.graph.last_event_of_thread(tid);
                            self.update_insertion_order(size - 1, ev, cur_insertion_order);
                            let new_insertion_order =
                                self.state.stack[size - 1].event_info.insertion_order;
                            assert!(cur_insertion_order == new_insertion_order);
                            self.prepare_state_for_rebuild(); // force rebuild!
                        }
                        return TrustAction::Rebuild;
                    } else {
                        // We already rebuilt the graph above, all is good!
                    }
                }
                _ => {
                    panic!("unexpected type of event found in the stack of TruST.");
                }
            }
        }
        // We finished! Everything is verified!
        // Sucessfully terminate the algorithm :D YAY
        TrustAction::End
    }

    /// For reconstruction we know that the reads are read-from the
    /// latest write, and the every new write is co-maximal.
    /// We must update the `insertion_order` to the previous value
    /// to preserve the `<_{G}` order.
    fn add_new_event_in_reconstruction_mode(
        &mut self,
        tid: usize,
        ev_info: EventInfo,
        old_insertion_order: u64,
    ) {
        let addr = ev_info.addr.unwrap();
        let lastest_write = match self.last_write_to_addr.get(&addr) {
            Some(w) => *w,
            None => EventId::new(0, 0),
        };
        let ev = match ev_info.event_type {
            EventType::Read => self
                .state
                .graph
                .add_new_read_event(tid, ev_info, lastest_write),
            EventType::Write => {
                let ev = self
                    .state
                    .graph
                    .add_new_write_event(tid, ev_info, lastest_write);
                self.last_write_to_addr.insert(addr, ev);
                ev
            }
            _ => {
                panic!(
                    "Unexpected type {:?}. Expected only READ or WRITE",
                    ev_info.event_type
                );
            }
        };
        self.state
            .graph
            .update_insertion_order(ev, old_insertion_order);
    }

    /// Gets the ExecutionGraph from the stack
    /// by making a full simulation of all it's events.
    /// This adds a lot of overhead, and should be avoided.
    /// At least we only call this in case of backwards revisits...
    /// TODO [optimization idea]: We can still make it better by only
    /// calling in case of the last co-edge option of each read.
    fn rebuild_graph_from_stack(&self) -> ExecutionGraph {
        // this sucks honestly...
        // isnt there a smarter way to do this?
        let mut g = ExecutionGraph::new();
        for item in &self.state.stack {
            let ev: EventId = match item.event_info.event_type {
                EventType::Read => {
                    let w_ev = item.write_id_from_read_iterator();
                    // Update the `is_rmw` value of CAS, if needed
                    let read_ev_info = item.event_info;
                    if EventInfo::is_cas(&read_ev_info) {
                        let addr = read_ev_info.addr.unwrap();
                        let init_val = self.cached_address_info.get(&addr).unwrap().init_value;
                        // We must update the init value for this mocked graph.
                        g.update_init_value_if_not_set(addr, init_val);
                    }
                    g.add_new_read_event(item.tid, read_ev_info, w_ev)
                }
                EventType::Write => {
                    match item.iterator {
                        WriteBackwardsRevisitIterator(read_id, write_id) => {
                            // backwards revisit
                            let w_ev = g.perform_backwards_revist(
                                read_id,
                                write_id,
                                item.tid,
                                item.event_info,
                            );
                            let mut read_ev_info = g.get_event_info(&read_id);
                            if EventInfo::is_cas(&read_ev_info) {
                                let write_ev_info = g.get_event_info(&w_ev);
                                assert!(
                                    write_ev_info.val != EventValue::WriteValue(ValuesTypes::None)
                                );
                                event_utils::update_rmw_status_of_cas(
                                    &mut read_ev_info,
                                    write_ev_info.val,
                                    ValuesTypes::None,
                                );
                                g.update_is_rmw(&read_id, read_ev_info.is_rmw);
                            }
                            w_ev
                        }
                        WriteCOIterator(write_id) => {
                            // add the write as co-after item.write_id
                            g.add_new_write_event(item.tid, item.event_info, write_id)
                        }
                        _ => panic!("Unexpected iterator for a Write"),
                    }
                }
                _ => {
                    panic!(
                        "Unexpected EventType {:?}. Expected Read or a Write",
                        item.event_info.event_type
                    );
                }
            };
            // we must update the insertion_order to preserve the `<_{G}` order.
            g.update_insertion_order(ev, item.event_info.insertion_order);
        }
        g
    }

    /// Updates the `insertion_order` of `ev` for both stack and graph.
    fn update_insertion_order(
        &mut self,
        stack_position: usize,
        ev: EventId,
        old_insertion_order: u64,
    ) {
        self.state.stack[stack_position].event_info.insertion_order = old_insertion_order;
        self.state
            .graph
            .update_insertion_order(ev, old_insertion_order);
    }

    fn new_read_stack_item(&mut self, tid: usize, ev_info: EventInfo) -> TrustStackFrame {
        let iter = TrustEventIterator::new(&mut self.state.graph, tid, ev_info);
        TrustStackFrame::new(tid, ev_info, iter)
    }
    fn new_write_stack_item(&mut self, tid: usize, ev_info: EventInfo) -> TrustStackFrame {
        let iter = TrustEventIterator::new(&mut self.state.graph, tid, ev_info);
        TrustStackFrame::new(tid, ev_info, iter)
    }

    pub fn signal_expecting_rmw_write(
        &mut self,
        tid: TaskId,
        addr: Address,
        lenght: AddrSize,
        val: EventValue,
    ) {
        self.expected_rmw_write = Some((tid, addr, lenght, val));
    }
    pub fn expects_rmw_write(&self) -> bool {
        self.expected_rmw_write.is_some()
    }
    pub fn get_next_rmw_write(&self) -> (TaskId, Address, AddrSize, EventValue) {
        self.expected_rmw_write.unwrap()
    }
    fn is_last_event_of_task_in_reconstruction(&self, tid: TaskId, pos: usize) -> bool {
        !self.state.rebuild_trace[pos + 1..]
            .iter()
            .any(|(ev_tid, _ev_info)| *ev_tid == tid)
    }

    /// Sanity check: we check that the graph generated by following the stack
    /// is the same as the current one.
    /// This way, if a modification breaks this invariant we should know.
    fn sanity_check_stack_graph(&self) {
        let graph_from_stack = self.rebuild_graph_from_stack();
        assert!(ExecutionGraph::graphs_are_equivalent(
            &graph_from_stack,
            &self.state.graph
        ));
    }
    fn sanity_check_graph(&self) {
        assert!(self.state.graph.insertion_order_is_unique());
    }

    pub fn new() -> Self {
        Self {
            state: TrustState::new(),
            last_write_to_addr: FxHashMap::default(),
            virtual_address_mapper: VirtualAddressMapper::new(0),
            expected_rmw_write: None,
            skip_failed_cas_tid_in_rebuild: None,
            failed_cas_tid: None,
            cached_address_info: FxHashMap::default(),
            total_graphs: 0,
            test_mode: false,
        }
    }

    /// Gets the initial value at the address
    /// ## Panic
    /// Panics if un-set.
    pub fn get_init_value_of_address(&self, addr: Address) -> ValuesTypes {
        self.cached_address_info[&addr].init_value
    }

    /// [TODO] What about mixed-size access?
    /// [TODO]: Update this in the TruSt handler! Could be initialized with trash value...
    /// This is called right BEFORE this address has an event that will access it.
    /// In case of a rebuild, we assert that this address is from the event
    /// immediate next one in the trace.
    pub fn update_init_value_if_not_set(&mut self, real_addr: Address, val: ValuesTypes) {
        let addr = self.get_virtual_addr_or_create(real_addr);
        // update on the graph
        if self.cached_address_info.contains_key(&addr) {
            return;
        }
        self.cached_address_info.insert(addr, AddressInfo::new(val));
        self.state.graph.update_init_value_if_not_set(addr, val);
    }

    pub fn get_real_addr_from_virtual(&self, virtual_addr: &Address) -> Address {
        self.virtual_address_mapper.get_real_address(virtual_addr)
    }

    /// Returns the virtual address mapping of `real_addr`
    ///
    /// If it doesnt exist it will create such a mapping.
    /// If the algorithm is in rebuild, it will extract the virtual_address from
    /// the stack.
    fn get_virtual_addr_or_create(&mut self, real_addr: Address) -> Address {
        if let Some(addr) = self.virtual_address_mapper.get_virtual_address(&real_addr) {
            addr
        } else if self.is_rebuilding() {
            // We still are adding events. So, the real address must be mapped
            // in the first event of the rebuild trace.
            let expec_ev_info = self.state.rebuild_trace[self.state.rebuild_trace_ptr + 1].1;
            self.virtual_address_mapper
                .add_mapping_in_rebuild(real_addr, expec_ev_info.addr.unwrap())
        } else {
            self.virtual_address_mapper
                .get_virtual_address_or_create(&real_addr)
        }
    }

    fn update_rmw_state(&self, ev_info: &mut EventInfo, iter: TrustEventIterator) {
        let write_id = iter.write_id_from_read_iterator();
        self.update_rmw_status_of_cas(ev_info, write_id);
    }

    fn update_rmw_status_of_cas(&self, ev_info: &mut EventInfo, write_id: EventId) {
        let addr = ev_info.addr.unwrap();
        let write_val = self.state.graph.get_event_info(&write_id).val;
        let init_val = self.get_init_value_of_address(addr);
        event_utils::update_rmw_status_of_cas(ev_info, write_val, init_val)
    }

    fn update_rmw_status_for_tests_in_rebuild(&self, ev_info: &mut EventInfo) {
        if EventInfo::is_cas(ev_info) {
            let lastest_write = match self.last_write_to_addr.get(&ev_info.addr.unwrap()) {
                Some(w) => *w,
                None => EventId::new(0, 0),
            };
            self.update_rmw_status_of_cas(ev_info, lastest_write);
        }
    }

    pub fn get_total_number_of_graphs(&self) -> u64 {
        self.total_graphs
    }

    // DEBUG
    pub fn print_graph_trace(&self) {
        trace!("Trace:");
        self.state.graph.print_trace();
    }
    pub fn print_reconstruction_trace(&self) {
        trace!("Reconstruction trace:");
        for (tid, ev_info) in &self.state.rebuild_trace {
            if !ev_info.is_atomic {
                continue;
            }
            trace!(
                "{:?} {:?}{:?}, insertion_order = {:?}, is_atomic = {:?}, addr = {:?}",
                tid,
                ev_info.event_type,
                if ev_info.is_rmw {
                    "_RMW".to_string()
                } else {
                    "".to_string()
                },
                ev_info.insertion_order,
                ev_info.is_atomic,
                ev_info.addr
            );
        }
    }
    pub fn print_fr_edges(&self) {
        self.state.graph.print_fr_edges();
    }
    pub fn print_edges(&self) {
        self.state.graph.print_edges();
        self.state.graph.print_fr_edges();
        dbg!(self.state.graph.is_consistent_sc());
    }

    pub fn serialize(&self) -> String {
        serde_json::to_string(&self).expect("valid JSON")
    }

    fn deserialize(serialized_string: String) -> TruST {
        serde_json::from_str(&serialized_string).expect("Valid JSON")
    }

    /// Public interface to desirialize TruST and also initialize properly its content
    /// This will:
    /// - Clean unused address from cache
    /// - Initialize the ExecutionGraph properly
    /// - Update initial values in the ExecutionGraph
    pub fn deserialize_and_init(serialized_string: String) -> TruST {
        let mut trust: TruST = TruST::deserialize(serialized_string);
        trust.init_content_after_serialization();
        trust
    }
    /// Initializes the ExecutionGraph and cleans the cached information of
    /// the virtual accesses
    fn init_content_after_serialization(&mut self) {
        self.cached_address_info = self.get_active_cached_address_info();
        self.state.graph.init_after_deserialization();
        // initialize values for the graph
        for (addr, addr_info) in &self.cached_address_info {
            self.state
                .graph
                .update_init_value_if_not_set(*addr, addr_info.init_value);
        }
    }

    /// Returns a HashMap of the cached information of the virtual addresses
    /// that still have some record on the stack.
    /// This garbage collection ensures that the memory of TruST is
    /// still polynomial in the size of the program.
    fn get_active_cached_address_info(&self) -> FxHashMap<Address, AddressInfo> {
        let mut active_addr: FxHashSet<Address> = FxHashSet::default();
        let mut cached_info: FxHashMap<Address, AddressInfo> = FxHashMap::default();
        for stack_frame in &self.state.stack {
            if let Some(addr) = &stack_frame.event_info.addr {
                if active_addr.insert(*addr) {
                    let addr_info = self.cached_address_info[addr];
                    cached_info.insert(*addr, addr_info);
                }
            }
        }
        cached_info
    }

    // DEBUG

    pub fn print_cached_address_info(&self) {
        for (addr, addr_info) in &self.cached_address_info {
            dbg!(addr, addr_info.init_value);
        }
    }

    // SANITY CHECKS
    fn assert_equals_expected_rmw_write(&self, tid: &usize, ev_info: &EventInfo) {
        let (exp_tid, addr, length, _val) = self.expected_rmw_write.unwrap();
        assert!(
            tid == &usize::from(exp_tid)
                && addr == ev_info.addr.unwrap()
                && length == ev_info.addr_length.unwrap()
                && ev_info.is_rmw
                && ev_info.event_type == EventType::Write
        );
    }

    /// Makes sanity checks to assert that `(tid, ev_info)` and
    /// `(expec_tid, expec_ev_info)` are equivalent.
    ///
    /// This should have no extra logic beside assertions.
    fn assert_event_equals_expected_in_rebuild(
        &self,
        tid: &usize,
        ev_info: &EventInfo,
        expec_tid: &TaskId,
        expec_ev_info: &EventInfo,
    ) {
        if let Some(tid) = self.skip_failed_cas_tid_in_rebuild {
            assert!(&tid != expec_tid);
        }
        // assert the added event is equal to the
        // desired in the reconstruction trace
        assert!(&TaskId::new(*tid as u64) == expec_tid);
        assert!(ev_info.event_type == expec_ev_info.event_type);
        assert!(ev_info.is_atomic == expec_ev_info.is_atomic);
        // only reads can change `rmw` status
        assert!(ev_info.is_rmw == expec_ev_info.is_rmw || ev_info.event_type == EventType::Read);
        if ev_info.is_rmw && !expec_ev_info.is_rmw {
            assert!(self
                .is_last_event_of_task_in_reconstruction(*expec_tid, self.state.rebuild_trace_ptr));
        }
    }
}
