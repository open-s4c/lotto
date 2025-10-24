use super::*;
use rand::Rng;
use std::{num::NonZeroUsize, time::Instant};

#[allow(dead_code)]
fn mocked_atomic_read(addr: Address) -> EventInfo {
    EventInfo::new(
        0,
        Some(addr),
        Some(AddrSize::new(4)),
        EventType::Read,
        true,
        /* is_rmw */ false,
        EventValue::NoneValue,
    )
}
#[allow(dead_code)]
fn mocked_atomic_rmw_read(addr: Address) -> EventInfo {
    EventInfo::new(
        0,
        Some(addr),
        Some(AddrSize::new(4)),
        EventType::Read,
        true,
        /* is_rmw */ true,
        EventValue::NoneValue,
    )
}
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
fn mocked_atomic_write(addr: Address) -> EventInfo {
    EventInfo::new(
        0,
        Some(addr),
        Some(AddrSize::new(4)),
        EventType::Write,
        true,
        /* is_rmw */ false,
        EventValue::NoneValue,
    )
}
#[allow(dead_code)]
fn mocked_atomic_rmw_write(addr: Address) -> EventInfo {
    EventInfo::new(
        0,
        Some(addr),
        Some(AddrSize::new(4)),
        EventType::Write,
        true,
        /* is_rmw */ true,
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
#[allow(dead_code)]
fn mocked_no_event() -> EventInfo {
    EventInfo::new(
        0,
        None,
        None,
        EventType::None,
        false,
        /* is_rmw */ false,
        EventValue::NoneValue,
    )
}

#[test]
fn test_execution_graph_sc() {
    let mut exec_g = ExecutionGraph::new();
    let addr1 = Address::new(NonZeroUsize::new(1).unwrap());
    let addr2 = Address::new(NonZeroUsize::new(2).unwrap());
    let init_id = EventId::new(0, 0);
    // Mocked test of program:
    // (1) R(addr1)  || (3) R(addr2)
    // (2) W(addr2)  || (4) W(addr1)

    let ev_info1 = mocked_read(addr1);
    let ev1 = exec_g.add_new_read_event(1, ev_info1, init_id);
    assert!(exec_g.is_consistent_sc());

    let ev_info2 = mocked_write(addr2);
    let ev2 = exec_g.add_new_write_event(1, ev_info2, init_id);
    assert!(exec_g.is_consistent_sc());

    let ev_info3 = mocked_read(addr2);
    let ev3 = exec_g.add_new_read_event(2, ev_info3, ev2);
    assert!(exec_g.is_consistent_sc());

    let ev_info4 = mocked_write(addr1);
    let ev4 = exec_g.add_new_write_event(2, ev_info4, init_id);
    assert!(exec_g.is_consistent_sc());

    // TODO: fix this test
    // assert!(exec_g.check_for_errors_sc(&ev4)); // should throw error due to data-race

    // should have a cycle now because we would have init_id ->_co ev5 ->_co ev4
    // but also ev5 ->_po ev4
    let ev_info5 = mocked_write(addr2);
    let ev5 = exec_g.add_new_write_event(2, ev_info5, init_id);
    // println!("Test if is consistent with cycle");
    assert!(!exec_g.is_consistent_sc());

    // remove event 5 and check if it is back to being consistent.
    exec_g.remove_set_of_events(&vec![ev5]);
    assert!(exec_g.is_consistent_sc());

    exec_g.remove_set_of_events(&vec![ev3, ev4]);
    let ev_info3 = mocked_read(addr2);
    let ev3 = exec_g.add_new_read_event(2, ev_info3, ev2);
    assert!(exec_g.is_consistent_sc());

    let ev_info4 = mocked_write(addr1);
    // cant backward revisit because ev2 is in porf-prefix
    assert!(!exec_g.can_backwards_revisit(&ev1, 2, ev_info4));
    exec_g.remove_set_of_events(&vec![ev3]);

    // now ev3 reads from the INIT state and we can backward-revisit
    let ev_info3 = mocked_read(addr2);
    let ev3 = exec_g.add_new_read_event(2, ev_info3, init_id);
    assert!(exec_g.is_consistent_sc());

    let ev_info4 = mocked_write(addr1);
    assert!(exec_g.can_backwards_revisit(&ev1, 2, ev_info4));

    // empty graph should be consistent:
    exec_g.remove_set_of_events(&vec![ev1, ev2, ev3]);
    assert!(exec_g.is_consistent_sc());
}

#[test]
fn test_backwards_revisits() {
    // Some tests for the program R + W + W
    // R(x) || W(x) || W(x)
    // check that we cant undo a backwards revisit
    let mut exec_g = ExecutionGraph::new();
    let addr1 = Address::new(NonZeroUsize::new(1).unwrap());
    let init_id = EventId::new(0, 0);

    let ev_info1 = mocked_atomic_read(addr1);
    let r1 = exec_g.add_new_read_event(1, ev_info1, init_id);
    assert!(exec_g.is_consistent_sc());

    let ev_info2 = mocked_atomic_write(addr1);
    assert!(exec_g.can_backwards_revisit(&r1, 2, ev_info2));

    assert!(exec_g.has_edge(init_id, r1, EdgeType::RF));
    let w1 = exec_g.perform_backwards_revist(r1, init_id, 2, ev_info2);
    // assert the RF edge changed:
    assert!(!exec_g.has_edge(init_id, r1, EdgeType::RF));
    assert!(exec_g.has_edge(w1, r1, EdgeType::RF));

    assert!(exec_g.is_consistent_sc());
    assert_equals_sorted(&exec_g.get_valid_ids(), &vec![init_id, r1, w1]);

    let ev_info3 = mocked_atomic_write(addr1);
    // now try to undo the backwards revisit (should fail)
    assert!(!exec_g.can_backwards_revisit(&r1, 3, ev_info3));
    let w2 = exec_g.add_new_write_event(3, ev_info3, init_id);
    assert!(exec_g.is_consistent_sc());
    // assert the CO edges are from init_id -> w2 -> w1:
    assert!(exec_g.sc_trace_from_graph() == vec![init_id, w2, w1, r1]);
    assert!(exec_g.has_edge(init_id, w2, EdgeType::CO));
    assert!(exec_g.has_edge(w2, w1, EdgeType::CO));
    assert!(exec_g.has_edge(w1, r1, EdgeType::RF));

    // start over:
    exec_g.remove_set_of_events(&vec![r1, w1, w2]);
    assert!(exec_g.is_consistent_sc());

    let r1 = exec_g.add_new_read_event(1, ev_info1, init_id);
    assert!(exec_g.is_consistent_sc());
    let w1 = exec_g.add_new_write_event(2, ev_info2, init_id);
    assert!(exec_g.is_consistent_sc());

    // now should be able to revisit:
    assert!(exec_g.can_backwards_revisit(&r1, 3, ev_info3));

    // this should fail because w1 will be deleted with the revisit
    assert!(!exec_g.can_add_co_order_in_backwards_revist(w1, r1, 3, ev_info3));

    let w2 = exec_g.perform_backwards_revist(r1, init_id, 3, ev_info3);
    assert!(exec_g.is_consistent_sc());
    assert!(!exec_g.get_valid_ids().contains(&w1)); // deleted

    // re-insert deleted event
    let w1 = exec_g.add_new_write_event(2, ev_info2, init_id);

    // assert the CO edges are from init_id -> w1 -> w2:
    assert!(exec_g.sc_trace_from_graph() == vec![init_id, w1, w2, r1]);
    assert!(exec_g.has_edge(init_id, w1, EdgeType::CO));
    assert!(exec_g.has_edge(w1, w2, EdgeType::CO));
    assert!(exec_g.has_edge(w2, r1, EdgeType::RF));

    exec_g.remove_set_of_events(&vec![r1, w1, w2]);
    assert!(exec_g.is_consistent_sc());
}

#[test]
fn test_backwards_revisits_2() {
    // Some tests for the program R + W + W
    // W(x) || R(x) || W(x)
    // check that we cant undo a backwards revisit
    let mut exec_g = ExecutionGraph::new();
    let addr1 = Address::new(NonZeroUsize::new(1).unwrap());
    let init_id = EventId::new(0, 0);

    let ev_info1 = mocked_atomic_write(addr1);
    let w1 = exec_g.add_new_write_event(1, ev_info1, init_id);
    assert!(exec_g.is_consistent_sc());

    let ev_info2 = mocked_atomic_read(addr1);
    let r2: EventId = exec_g.add_new_read_event(2, ev_info2, w1);
    assert!(exec_g.is_consistent_sc());

    let ev_info3 = mocked_atomic_write(addr1);
    assert!(exec_g.can_backwards_revisit(&r2, 3, ev_info3));
    let w3 = exec_g.perform_backwards_revist(r2, init_id, 3, ev_info3);
    assert!(exec_g.is_consistent_sc());

    exec_g.remove_set_of_events(&vec![r2, w1, w3]);
    // Not if it reads from INIT, we cant do the revisit anymore:
    let ev_info1 = mocked_atomic_write(addr1);
    let _w1 = exec_g.add_new_write_event(1, ev_info1, init_id);
    assert!(exec_g.is_consistent_sc());

    let ev_info2 = mocked_atomic_read(addr1);
    let r2: EventId = exec_g.add_new_read_event(2, ev_info2, init_id);
    assert!(exec_g.is_consistent_sc());

    let ev_info3 = mocked_atomic_write(addr1);
    assert!(!exec_g.can_backwards_revisit(&r2, 3, ev_info3));
}

#[test]
/// Simple test to ensure that there cant be any write in between the
/// read and write of a RMW.
fn test_atomicity_of_rmw() {
    let mut exec_g = ExecutionGraph::new();
    let addr1 = Address::new(NonZeroUsize::new(1).unwrap());
    let init_id = EventId::new(0, 0);

    let ev_info2 = mocked_atomic_write(addr1);
    let w2 = exec_g.add_new_write_event(2, ev_info2, init_id);
    assert!(exec_g.is_consistent_sc());

    let ev_info1 = mocked_atomic_rmw_read(addr1);
    let r1 = exec_g.add_new_read_event(1, ev_info1, init_id);
    assert!(exec_g.is_consistent_sc());

    let ev_info3 = mocked_atomic_rmw_write(addr1);
    let w1 = exec_g.add_new_write_event(1, ev_info3, w2);
    assert!(!exec_g.is_consistent_sc());
    assert!(!exec_g.atomicity_of_rmw_is_preserved()); // in particular this fails as well
    assert!(!exec_g.list_of_rmw_reads.is_empty());

    exec_g.remove_last_event(w1);

    let w1 = exec_g.add_new_write_event(1, ev_info3, init_id);
    assert!(exec_g.is_consistent_sc());

    exec_g.remove_set_of_events(&vec![r1, w1]);
    assert!(exec_g.list_of_rmw_reads.is_empty());
}

#[test]
fn test_backwards_revisits_r_plus_ww() {
    // Some tests for the program R + W, W
    // Rx | Wx
    //    | Wx
    // check that we cant undo a backwards revisit!
    let mut exec_g = ExecutionGraph::new();
    let addr1 = Address::new(NonZeroUsize::new(1).unwrap());
    let init_id = EventId::new(0, 0);

    let ev_info1 = mocked_atomic_read(addr1);
    let r1 = exec_g.add_new_read_event(1, ev_info1, init_id);
    assert!(exec_g.is_consistent_sc());

    let ev_info2 = mocked_atomic_write(addr1);
    assert!(exec_g.can_backwards_revisit(&r1, 2, ev_info2));
    assert!(exec_g.has_edge(init_id, r1, EdgeType::RF));

    let w1 = exec_g.perform_backwards_revist(r1, init_id, 2, ev_info2);
    // assert the RF edge changed:
    assert!(!exec_g.has_edge(init_id, r1, EdgeType::RF));
    assert!(exec_g.has_edge(w1, r1, EdgeType::RF));
    assert!(exec_g.is_consistent_sc());
    assert_equals_sorted(&exec_g.get_valid_ids(), &vec![init_id, r1, w1]);

    let ev_info3 = mocked_atomic_write(addr1);
    // now try to undo the backwards revisit (should succeded actually because it is the same thread)
    assert!(exec_g.can_backwards_revisit(&r1, 2, ev_info3));

    let w2 = exec_g.perform_backwards_revist(r1, w1, 2, ev_info3);
    assert!(exec_g.is_consistent_sc());
    // assert the CO edges are from init_id -> w1 -> w2:
    assert!(exec_g.has_edge(init_id, w1, EdgeType::CO));
    assert!(exec_g.has_edge(w1, w2, EdgeType::CO));
    assert!(exec_g.has_edge(w2, r1, EdgeType::RF));
    // OK
    exec_g.remove_set_of_events(&vec![r1, w1, w2]);
    // now start over and try to backwards-revisit from w2 to r1 WITHOUT first revisiting
    // from w1->r1 (should fail):
    let r1 = exec_g.add_new_read_event(1, ev_info1, init_id);
    assert!(exec_g.is_consistent_sc());
    assert!(exec_g.has_edge(init_id, r1, EdgeType::RF));
    let w1 = exec_g.add_new_write_event(2, ev_info2, init_id);
    // assert the RF edge changed:
    assert!(exec_g.has_edge(init_id, r1, EdgeType::RF));
    assert!(exec_g.is_consistent_sc());
    assert_equals_sorted(&exec_g.get_valid_ids(), &vec![init_id, r1, w1]);
    // now try to undo the backwards revisit. This time it must fail.
    assert!(!exec_g.can_backwards_revisit(&r1, 2, ev_info3));
}

#[test]
fn stress_test_execution_graph_sc() {
    use rand::rngs::StdRng;
    use rand::Rng;
    use rand::SeedableRng;

    for tc in 0..=20 {
        let mut random = StdRng::seed_from_u64(tc + 2000);
        if tc % 10 == 0 {
            println!("Running testcase #{tc}");
        }
        let mut g = ExecutionGraph::new();
        let mut list_of_active_events: Vec<EventId> = vec![EventId::new(0, 0)]; // INIT
        for operation in 0..=1000 {
            if list_of_active_events.len() > 30 {
                // remove some events:
                // let mx = list_of_active_events.len() / 2 + 1;
                // random.gen_range(0..=mx); // cant remove like this because you could
                // leave some R without a rf edge, due to backwards revisit. :(
                let qtd = list_of_active_events.len();
                let mut events_to_remove = Vec::new();
                for _ in 1..=qtd {
                    if list_of_active_events.len() == 1 {
                        break;
                    }
                    events_to_remove.push(list_of_active_events.pop().unwrap());
                }
                g.remove_set_of_events(&events_to_remove);
            }
            let rand_addr = random.gen_range(1..=4);
            let addr = Address::new(NonZeroUsize::new(rand_addr).unwrap());
            let tid: usize = random.gen_range(1..=4);

            if random.gen_bool(0.5) {
                // read
                let ev_info = if random.gen_bool(0.5) {
                    mocked_read(addr)
                } else {
                    mocked_atomic_read(addr)
                };
                let candidates_w = g.events_writing_to_addr_and_init(ev_info);
                let mut rf_w = get_random_write_event(&mut random, &candidates_w);

                let mut op = 0;
                while !g.can_add_rf_edge(rf_w, tid, ev_info) && op < 10 {
                    rf_w = get_random_write_event(&mut random, &candidates_w);
                    op += 1;
                }
                if g.can_add_rf_edge(rf_w, tid, ev_info) {
                    let ev = g.add_new_read_event(tid, ev_info, rf_w);
                    list_of_active_events.push(ev);
                }
            } else {
                // write
                let ev_info = if random.gen_bool(0.5) {
                    mocked_write(addr)
                } else {
                    mocked_atomic_write(addr)
                };

                // try to backwards revisit:
                let r = get_random_read_event(&mut random, &g.events_reading_from_addr(ev_info));

                let backwards_revisit = if let Some(r) = r {
                    let mut f = false;
                    if g.can_backwards_revisit(&r, tid, ev_info) {
                        let deleted_events = g.delete_events_of_backwards_revisit(tid, ev_info, r);
                        list_of_active_events.retain(|ev| !deleted_events.contains(ev));

                        let candidates_to_co = g.events_writing_to_addr_and_init(ev_info);
                        let mut prv_co = get_random_write_event(&mut random, &candidates_to_co);
                        while !g.can_add_co_order_in_backwards_revist(prv_co, r, tid, ev_info) {
                            prv_co = get_random_write_event(&mut random, &candidates_to_co);
                        }
                        let new_event =
                            g.add_new_write_event_backwards_revisit(r, prv_co, tid, ev_info);
                        // println!("revisit for {:?} and {:?}, prv_co = {:?}", r, new_event, prv_co);
                        list_of_active_events.push(new_event);
                        // println!("backwards revisit deleted {:?} events", deleted_events.len());
                        f = true;
                    }
                    f
                } else {
                    false
                };

                if !backwards_revisit {
                    let candidates_to_co = g.events_writing_to_addr_and_init(ev_info);
                    let mut prv_co = get_random_write_event(&mut random, &candidates_to_co);
                    let mut op = 0;
                    while !g.can_add_co_order(prv_co, tid, ev_info) && op < 10 {
                        prv_co = get_random_write_event(&mut random, &candidates_to_co);
                        op += 1;
                    }
                    if g.can_add_co_order(prv_co, tid, ev_info) {
                        let w = g.add_new_write_event(tid, ev_info, prv_co);
                        list_of_active_events.push(w);
                    }
                }
            }
            // check if it is still consistent
            if !g.is_consistent_sc() {
                g.print_edges();
                unreachable!();
            }
            // check for ids list
            assert_equals_sorted(&g.get_valid_ids(), &list_of_active_events);
            if operation % 100 == 0 && tc % 10 == 0 {
                println!("len = {:?}", list_of_active_events.len()); // just to have an idea of the size
            }
        }
    }
}

#[test]
/// Test to see if the function `is_consistent` is really O(N).
fn test_consistency_runtime() {
    let mut g = ExecutionGraph::new();
    let init_id = EventId::new(0, 0);
    let addr_1 = Address::new(NonZeroUsize::new(4).unwrap());
    let addr_2 = Address::new(NonZeroUsize::new(8).unwrap());

    let r_1 = mocked_atomic_read(addr_1);
    let r_2 = mocked_atomic_read(addr_2);
    let w_1 = mocked_atomic_write(addr_1);
    let w_2 = mocked_atomic_write(addr_2);
    let now = Instant::now();
    let tid = 1;
    for num in 0..100000 {
        let last_w = if num == 0 {
            init_id
        } else {
            EventId::new(tid, num * 2 - 2)
        };
        let w_ev = g.add_new_write_event(tid, w_1, last_w);
        g.add_new_read_event(tid, r_1, w_ev);
    }
    let tid = 2;
    for num in 0..100000 {
        let last_w = if num == 0 {
            init_id
        } else {
            EventId::new(tid, num * 2 - 2)
        };
        let w_ev = g.add_new_write_event(tid, w_2, last_w);
        g.add_new_read_event(tid, r_2, w_ev);
    }
    dbg!(now.elapsed());
    let now = Instant::now();
    assert!(g.is_consistent_sc());
    assert!(now.elapsed().as_secs() < 10);
    dbg!(now.elapsed());
}

#[test]
fn test_fr_edges() {
    // The function to get the `fr` edges must return the
    // first edge in `co` order, but also for the RMW it must add
    // an extra edge between R_1 -> R_{RMW}, in case R_1 ->_{fr} W_{RMW}
    // to avoid lotto mistakenly splitting the atomic operations when
    // generating a trace.
    let mut g = ExecutionGraph::new();
    let init_id = EventId::new(0, 0);
    let addr_1 = Address::new(NonZeroUsize::new(4).unwrap());
    let info_r1 = mocked_atomic_read(addr_1);
    let r1 = g.add_new_read_event(2, info_r1, init_id);

    let info_r_rmw = mocked_atomic_rmw_read(addr_1);
    let info_w_rmw = mocked_atomic_rmw_write(addr_1);
    let r_rmw = g.add_new_read_event(1, info_r_rmw, init_id);
    let w_rmw = g.add_new_write_event(1, info_w_rmw, init_id);

    let info_extra_w = mocked_atomic_write(addr_1);
    let _extra_w = g.add_new_write_event(1, info_extra_w, w_rmw);
    let edges = g.get_fr_edges();
    let expected_edges = [
        Edge::new(r1, r_rmw, EdgeType::FR),
        Edge::new(r1, w_rmw, EdgeType::FR),
        Edge::new(r_rmw, w_rmw, EdgeType::FR),
    ];
    assert!(
        expected_edges.iter().all(|e| edges.contains(e))
            && edges.iter().all(|e| expected_edges.contains(e))
    );
}

use rand::rngs::StdRng;
#[allow(dead_code)]
fn get_random_write_event(random: &mut StdRng, candidates: &Vec<EventId>) -> EventId {
    assert!(!candidates.is_empty());
    let id = random.gen_range(0..candidates.len());
    candidates[id]
}
#[allow(dead_code)]
fn get_random_read_event(random: &mut StdRng, candidates: &Vec<EventId>) -> Option<EventId> {
    if candidates.is_empty() {
        return None;
    }
    let id = random.gen_range(0..candidates.len());
    Some(candidates[id])
}

#[allow(dead_code)]
fn assert_equals_sorted(vec1: &Vec<EventId>, vec2: &Vec<EventId>) {
    let v1 = vec1.clone();
    let v2 = vec2.clone();
    for ev in v1 {
        assert!(v2.contains(&ev));
    }
}
