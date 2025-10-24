use crate::TruST;
use crate::TrustAction;
use event_utils::EventValue;
use event_utils::{EventInfo, EventType};
use lotto::engine::handler::AddrSize;
use lotto::engine::handler::Address;
use lotto::engine::handler::ValuesTypes;
use lotto::log::trace;
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
fn mocked_atomic_read(addr: Address) -> EventInfo {
    EventInfo::new(
        0,
        Some(addr),
        Some(AddrSize::new(4)),
        EventType::Read,
        /* is_atomic */ true,
        /* is_rmw */ false,
        EventValue::NoneValue,
    )
}
#[allow(dead_code)]
fn mocked_rmw_read(addr: Address, val: ValuesTypes) -> EventInfo {
    EventInfo::new(
        0,
        Some(addr),
        Some(AddrSize::new(4)),
        EventType::Read,
        /* is_atomic */ true,
        /* is_rmw */ true,
        EventValue::RmwValue(val),
    )
}
#[allow(dead_code)]
fn mocked_cas_read(addr: Address, expected: ValuesTypes, desired: ValuesTypes) -> EventInfo {
    EventInfo::new(
        0,
        Some(addr),
        Some(AddrSize::new(4)),
        EventType::Read,
        /* is_atomic */ true,
        /* is_rmw */ true,
        EventValue::CasValues(expected, desired),
    )
}
#[allow(dead_code)]
fn mocked_rmw_write(addr: Address, val: EventValue) -> EventInfo {
    EventInfo::new(
        0,
        Some(addr),
        Some(AddrSize::new(4)),
        EventType::Write,
        /* is_atomic */ true,
        /* is_rmw */ true,
        val,
    )
}
#[allow(dead_code)]
fn mocked_atomic_write(addr: Address, val: ValuesTypes) -> EventInfo {
    EventInfo::new(
        0,
        Some(addr),
        Some(AddrSize::new(4)),
        EventType::Write,
        true,
        /* is_rmw */ false,
        EventValue::WriteValue(val),
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
fn test_w_plus_w() {
    // we will simulate a simple program:
    //  Wx | Wx
    // It should have a total of 2 options for the co-edges.

    let addr_x = Address::new(NonZeroUsize::new(1).unwrap());
    let wx = mocked_write(addr_x);
    let moves: Vec<Vec<EventInfo>> = vec![vec![], vec![wx], vec![wx]];
    assert!(total_number_of_graphs(&moves) == 2);
}

#[test]
fn test_r_plus_w() {
    // we will simulate a simple program:
    //  Rx | Wx
    // It should have a total of 2 options for the co-edges.

    let addr_x = Address::new(NonZeroUsize::new(1).unwrap());
    let wx = mocked_write(addr_x);
    let rx = mocked_read(addr_x);
    let moves: Vec<Vec<EventInfo>> = vec![vec![], vec![rx], vec![wx]];
    assert!(total_number_of_graphs(&moves) == 2);
}

#[test]
fn test_w_plus_r_plus_w() {
    // we will simulate a simple program:
    //  Wx | Rx | Wx
    // It should have a total of 2 options for the co-edges and 2
    // for the rf edges. So 6 graphs in total.

    let addr_x = Address::new(NonZeroUsize::new(1).unwrap());
    let wx = mocked_write(addr_x);
    let rx = mocked_read(addr_x);
    let moves: Vec<Vec<EventInfo>> = vec![vec![], vec![wx], vec![rx], vec![wx]];
    assert!(total_number_of_graphs(&moves) == 6);
}

#[test]
fn test_w_plus_r_plus_r() {
    // we will simulate a simple program:
    //  Wx | Rx | Rx
    // It should have a total of 1 options for the co-edges and 4
    // for the rf edges. So 4 graphs in total.

    let addr_x = Address::new(NonZeroUsize::new(1).unwrap());
    let wx = mocked_write(addr_x);
    let rx = mocked_read(addr_x);
    let moves: Vec<Vec<EventInfo>> = vec![vec![], vec![wx], vec![rx], vec![rx]];
    assert!(total_number_of_graphs(&moves) == 4);
}

#[test]
fn test_ww_plus_r() {
    // we will simulate a simple program:
    //  Wx | Rx
    //  Wx |
    // It should have a total of 1 options for the co-edges and 3
    // for the rf edges. So 3 graphs in total.

    let addr_x = Address::new(NonZeroUsize::new(1).unwrap());
    let wx = mocked_write(addr_x);
    let rx = mocked_read(addr_x);
    let moves: Vec<Vec<EventInfo>> = vec![vec![], vec![wx, wx], vec![rx]];
    assert!(total_number_of_graphs(&moves) == 3);
}

#[test]
fn test_r_plus_ww() {
    // we will simulate a simple program:
    //  Rx | Wx
    //     | Wx
    // It should have a total of 1 options for the co-edges and 3
    // for the rf edges. So 3 graphs in total.
    // this is interesting because the first write does a backward revisit
    // and we want the second write to backward revisit on top of that.

    let addr_x = Address::new(NonZeroUsize::new(1).unwrap());
    let wx = mocked_write(addr_x);
    let rx = mocked_read(addr_x);
    let moves: Vec<Vec<EventInfo>> = vec![vec![], vec![rx], vec![wx, wx]];
    assert!(total_number_of_graphs(&moves) == 3);
}

#[test]
fn test_r_plus_wr_plus_w() {
    // we will simulate a simple program:
    //  Rx | Wx | Wx
    //     | Rx   |
    // It should have a total of 3 rf options if Wx (tid=2) comes after Wx (tid=3)
    // and 6 rf edge options else. So 9 graphs in total.

    let addr_x = Address::new(NonZeroUsize::new(1).unwrap());
    let wx = mocked_write(addr_x);
    let rx = mocked_read(addr_x);
    let moves: Vec<Vec<EventInfo>> = vec![vec![], vec![rx], vec![wx, rx], vec![wx]];
    assert!(total_number_of_graphs(&moves) == 9);
}

#[test]
fn test_rw_plus_wr() {
    // we will simulate a simple program:
    //  Rx | Wx
    //  Wx | Rx
    // It should have a total of 1 option for rf-edges in case Wx (tid=1)
    // comes before Wx (tid=2), and 4 options otherwise. So 5 graphs
    // in total.

    let addr_x = Address::new(NonZeroUsize::new(1).unwrap());
    let wx = mocked_write(addr_x);
    let rx = mocked_read(addr_x);
    let moves: Vec<Vec<EventInfo>> = vec![vec![], vec![rx, wx], vec![wx, rx]];
    assert!(total_number_of_graphs(&moves) == 5);
}

#[test]
fn test_rr_plus_ww() {
    // we will simulate a simple program:
    //  Rx | Wx
    //  Rx | Wx
    // It should have a total of 1 option for the co-edges and
    // 3 (init ->_rf r1) + 2 (w1 ->_rf r1) + 1 (w2 ->_rf r1) = 6
    // for the rf edges. So 6 graphs in total.

    let addr_x = Address::new(NonZeroUsize::new(1).unwrap());
    let wx = mocked_write(addr_x);
    let rx = mocked_read(addr_x);
    let moves: Vec<Vec<EventInfo>> = vec![vec![], vec![rx, rx], vec![wx, wx]];
    let total = total_number_of_graphs(&moves);
    assert!(total == 6);
}

#[test]
fn test_w_plus_w_plus_w_plus_w() {
    // we will simulate a simple program:
    //  Wx | Wx | Wx | Wx
    // It should have a total of 4! = 24 graphs.

    let addr_x = Address::new(NonZeroUsize::new(1).unwrap());
    let wx = mocked_write(addr_x);
    let moves: Vec<Vec<EventInfo>> = vec![vec![], vec![wx], vec![wx], vec![wx], vec![wx]];
    assert!(total_number_of_graphs(&moves) == 24);
}

#[test]
fn test_w_plus_w_plus_w_plus_w_plus_r() {
    // we will simulate a simple program:
    //  Wx | Wx | Wx | Wx | Rx
    // It should have a total of 4! * 5 = 120 graphs.

    let addr_x = Address::new(NonZeroUsize::new(1).unwrap());
    let wx = mocked_write(addr_x);
    let rx = mocked_read(addr_x);
    let moves: Vec<Vec<EventInfo>> = vec![vec![], vec![wx], vec![wx], vec![wx], vec![wx], vec![rx]];

    assert!(total_number_of_graphs(&moves) == 120);
}

#[test]
fn test_r_plus_w_plus_w_plus_w_plus_w() {
    // we will simulate a simple program:
    //  Rx | Wx | Wx | Wx | Wx
    // It should have a total of 4! * 5 = 120 graphs.

    let addr_x = Address::new(NonZeroUsize::new(1).unwrap());
    let wx = mocked_write(addr_x);
    let rx = mocked_read(addr_x);
    let moves: Vec<Vec<EventInfo>> = vec![vec![], vec![rx], vec![wx], vec![wx], vec![wx], vec![wx]];
    assert!(total_number_of_graphs(&moves) == 120);
}

#[test]
fn test_rrr_plus_www_plus_rrr() {
    // we will simulate a simple program:
    //  Rx | Wx | Rx
    //  Rx | Wx | Rx
    //  Rx | Wx | Rx
    // It should have a total of ((4+3+2+1)+(3+2+1)+(2+1)+(1))^2 = 20^2 = 400 graphs

    let addr_x = Address::new(NonZeroUsize::new(1).unwrap());
    let wx = mocked_write(addr_x);
    let rx = mocked_read(addr_x);
    let moves: Vec<Vec<EventInfo>> =
        vec![vec![], vec![rx, rx, rx], vec![wx, wx, wx], vec![rx, rx, rx]];
    assert!(total_number_of_graphs(&moves) == 400);
}

#[test]
fn test_wxry_plus_wyrxwy() {
    // we will simulate a simple program:
    //  Wx | Wy
    //  Ry | Rx
    //     | Wy
    // If Rx reads from Wx, then we have 3 options for Ry (INIT, Wy_1 or Wy_2)
    // If Rx reads from Init, we have 2 more options for Ry (Wy_1 or Wy_2). It
    // cant read from INIT because Wy_1 happens before Rx.
    // In total we have 5 graphs.

    let addr_x = Address::new(NonZeroUsize::new(1).unwrap());
    let addr_y = Address::new(NonZeroUsize::new(2).unwrap());
    let wx = mocked_write(addr_x);
    let wy = mocked_write(addr_y);
    let rx = mocked_read(addr_x);
    let ry = mocked_read(addr_y);
    let moves: Vec<Vec<EventInfo>> = vec![vec![], vec![wx, ry], vec![wy, rx, wy]];
    assert!(total_number_of_graphs(&moves) == 5);
}

#[test]
fn test_big_case_0() {
    // Total number of graphs was found by running a test in genMC.

    let addr_x = Address::new(NonZeroUsize::new(1).unwrap());
    let rx = mocked_read(addr_x);
    let wx = mocked_write(addr_x);

    let t1 = vec![rx];
    let t2 = vec![wx, wx, wx, wx, wx];
    let t3 = vec![wx, wx, wx, wx, wx];

    let moves: Vec<Vec<EventInfo>> = vec![vec![], t1, t2, t3];
    assert!(total_number_of_graphs(&moves) == 2772);
}

#[test]
fn test_big_case_1() {
    // Total number of graphs was found by running a test in genMC.

    let addr_x = Address::new(NonZeroUsize::new(1).unwrap());
    let rx = mocked_read(addr_x);
    let wx = mocked_write(addr_x);

    let t1 = vec![rx, rx, rx];
    let t2 = vec![wx, wx, wx];
    let t3 = vec![wx, wx, wx, wx];

    let moves: Vec<Vec<EventInfo>> = vec![vec![], t1, t2, t3];
    assert!(total_number_of_graphs(&moves) == 4200);
}

#[test]
fn test_big_case_2() {
    // Total number of graphs was found by running a test in genMC.

    let addr_x = Address::new(NonZeroUsize::new(1).unwrap());
    let addr_y = Address::new(NonZeroUsize::new(2).unwrap());
    let addr_z = Address::new(NonZeroUsize::new(3).unwrap());
    let addr_w = Address::new(NonZeroUsize::new(4).unwrap());
    let rx = mocked_read(addr_x);
    let ry = mocked_read(addr_y);
    let rz = mocked_read(addr_z);
    let rw = mocked_read(addr_w);

    let wx = mocked_write(addr_x);
    let wy = mocked_write(addr_y);
    let wz = mocked_write(addr_z);
    let ww = mocked_write(addr_w);

    let t1 = vec![rx, ry, rz, rw, rx, ry, rz];
    let t2 = vec![rz, ry, rx, rw];
    let t3 = vec![wx, wy, wx];
    let t4 = vec![wz, ww, wz];

    let moves: Vec<Vec<EventInfo>> = vec![vec![], t1, t2, t3, t4];
    assert!(total_number_of_graphs(&moves) == 2547);
}

#[test]
fn test_big_case_3() {
    // Total number of graphs was found by running a test in genMC.

    let addr_x = Address::new(NonZeroUsize::new(1).unwrap());
    let addr_y = Address::new(NonZeroUsize::new(2).unwrap());
    let addr_z = Address::new(NonZeroUsize::new(3).unwrap());
    let addr_w = Address::new(NonZeroUsize::new(4).unwrap());
    let rx = mocked_read(addr_x);
    let ry = mocked_read(addr_y);
    let rz = mocked_read(addr_z);
    let rw = mocked_read(addr_w);

    let wx = mocked_write(addr_x);
    let wy = mocked_write(addr_y);
    let wz = mocked_write(addr_z);
    let ww = mocked_write(addr_w);

    let t1 = vec![rx, wy, rz, ww];
    let t2 = vec![wx, ry, wz, rw];
    let t3 = vec![wx, wy, wz];
    let t4 = vec![ww, wz, wy, ry];

    let moves: Vec<Vec<EventInfo>> = vec![vec![], t1, t2, t3, t4];
    assert!(total_number_of_graphs(&moves) == 7800);
}

#[test]
fn test_big_case_4() {
    // Total number of graphs was found by running a test in genMC.

    let addr_x = Address::new(NonZeroUsize::new(1).unwrap());
    let addr_y = Address::new(NonZeroUsize::new(2).unwrap());
    let rx = mocked_read(addr_x);
    let ry = mocked_read(addr_y);

    let wx = mocked_write(addr_x);
    let wy = mocked_write(addr_y);

    let t1 = vec![rx, ry, rx, ry];
    let t2 = vec![rx, ry, wx, wy];
    let t3 = vec![wx, wy];
    let t4 = vec![wx, wy];

    let moves: Vec<Vec<EventInfo>> = vec![vec![], t1, t2, t3, t4];
    assert!(total_number_of_graphs(&moves) == 8352);
}

#[test]
fn test_big_case_5() {
    // Total number of graphs was found by running a test in genMC.
    let addr_x = Address::new(NonZeroUsize::new(1).unwrap());
    let rx = mocked_read(addr_x);

    let wx = mocked_write(addr_x);

    let t1 = vec![wx, rx, wx, rx, wx];
    let t2 = vec![wx, rx, wx, rx];
    let t3 = vec![wx, rx, wx];

    let moves: Vec<Vec<EventInfo>> = vec![vec![], t1, t2, t3];
    assert!(total_number_of_graphs(&moves) == 11680);
}

/// Test in the case of a lot of writes and 1 read.
#[test]
fn test_big_lots_of_writes() {
    // Total number of options is 9!/(3!)^3 = 1680 for the writes,
    // times 10 options for the read. In total, 16800 graphs possible.
    let addr_x = Address::new(NonZeroUsize::new(1).unwrap());
    let rx = mocked_read(addr_x);

    let wx = mocked_write(addr_x);

    let t1 = vec![wx, wx, wx];
    let t2 = vec![rx];
    let t3 = vec![wx, wx, wx];
    let t4 = vec![wx, wx, wx];

    let moves: Vec<Vec<EventInfo>> = vec![vec![], t1, t2, t3, t4];
    assert!(total_number_of_graphs(&moves) == 16800);
}

/// Test in the case of a lot of reads.
#[test]
fn test_big_lots_of_reads() {
    // Total number of options is 9!/(3!)^3 = 1680 for the writes,
    // times 10 options for the read. In total, 16800 graphs possible.
    let addr_x = Address::new(NonZeroUsize::new(1).unwrap());
    let rx = mocked_read(addr_x);

    let wx = mocked_write(addr_x);

    let t1 = vec![wx, rx];
    let t2 = vec![rx, rx, rx, rx, rx];
    let t3 = vec![rx, wx, rx];
    let t4 = vec![rx, wx, rx, rx];

    let moves: Vec<Vec<EventInfo>> = vec![vec![], t1, t2, t3, t4];
    assert!(total_number_of_graphs(&moves) == 10416);
}

#[test]
fn test_rr_plus_ww_plus_w() {
    // Simulates the program:
    // Rx | Wx | Wx
    // Rx | Wx |
    // Total number of graphs was found by running a test in genMC.

    let addr_x = Address::new(NonZeroUsize::new(1).unwrap());
    let rx = mocked_read(addr_x);
    let wx = mocked_write(addr_x);
    let t1 = vec![rx, rx];
    let t2 = vec![wx, wx];
    let t3 = vec![wx];

    let moves: Vec<Vec<EventInfo>> = vec![vec![], t1, t2, t3];
    assert!(total_number_of_graphs(&moves) == 30);
}

/// Same as above but with another order.
#[test]
fn test_ww_plus_rr_plus_w() {
    // Simulates the program:
    // Wx | Rx | Wx
    // Wx | Rx |
    // Total number of graphs was found by running a test in genMC.

    let addr_x = Address::new(NonZeroUsize::new(1).unwrap());
    let rx = mocked_read(addr_x);
    let wx = mocked_write(addr_x);
    let t1 = vec![wx, wx];
    let t2 = vec![rx, rx];
    let t3 = vec![wx];

    let moves: Vec<Vec<EventInfo>> = vec![vec![], t1, t2, t3];
    assert!(total_number_of_graphs(&moves) == 30);
}

#[test]
fn test_www_plus_www_plus_rrr() {
    // Simulates the program:
    // Wx | Wx | Rx
    // Wx | Wx | Rx
    // Wx | Wx | Rx
    // Total number of graphs was found by running a test in genMC.
    let addr_x = Address::new(NonZeroUsize::new(1).unwrap());
    let rx = mocked_read(addr_x);
    let wx = mocked_write(addr_x);
    let t1 = vec![wx, wx, wx];
    let t2 = vec![wx, wx, wx];
    let t3 = vec![rx, rx, rx];

    let moves: Vec<Vec<EventInfo>> = vec![vec![], t1, t2, t3];
    assert!(total_number_of_graphs(&moves) == 1680);
}

#[test]
fn test_rr_plus_w_plus_w() {
    let addr_x = Address::new(NonZeroUsize::new(1).unwrap());
    let rx = mocked_read(addr_x);
    let wx = mocked_write(addr_x);

    let t1 = vec![rx, rx];
    let t2 = vec![wx];
    let t3 = vec![wx];

    let moves: Vec<Vec<EventInfo>> = vec![vec![], t1, t2, t3];
    assert!(total_number_of_graphs(&moves) == 12);
}
#[test]
fn test_rmw_0() {
    let addr_x = Address::new(NonZeroUsize::new(1).unwrap());
    let rmw_x: EventInfo = mocked_rmw_read(addr_x, ValuesTypes::U32(1));

    let t1 = vec![rmw_x];
    let t2 = vec![rmw_x];

    let moves: Vec<Vec<EventInfo>> = vec![vec![], t1, t2];
    assert!(total_number_of_graphs(&moves) == 2);
}

#[test]
fn test_rmw_1() {
    let addr_x = Address::new(NonZeroUsize::new(1).unwrap());
    let rmw_x: EventInfo = mocked_rmw_read(addr_x, ValuesTypes::U32(1));
    let wx = mocked_atomic_write(addr_x, ValuesTypes::U32(1));

    let t1 = vec![rmw_x];
    let t2 = vec![wx];

    let moves: Vec<Vec<EventInfo>> = vec![vec![], t1, t2];
    assert!(total_number_of_graphs(&moves) == 2);
}
#[test]
fn test_rmw_2() {
    // Simulates the program:
    // RMW | RMW
    //     | RMW
    // Total number of graphs is the same as the total number of traces:
    // 3!/2! = 3.

    let addr_x = Address::new(NonZeroUsize::new(1).unwrap());
    let rmw_x: EventInfo = mocked_rmw_read(addr_x, ValuesTypes::U32(1));

    let t1 = vec![rmw_x];
    let t2 = vec![rmw_x, rmw_x];

    let moves: Vec<Vec<EventInfo>> = vec![vec![], t1, t2];
    assert!(total_number_of_graphs(&moves) == 3);
}
#[test]
fn test_rmw_3() {
    // Simulates the program:
    // RMW | RMW
    // RMW | RMW
    // RMW | RMW
    // Total number of graphs is the same as the total number of traces:
    // 6!/(3! * 3!) = 20

    let addr_x = Address::new(NonZeroUsize::new(1).unwrap());
    let rmw_x: EventInfo = mocked_rmw_read(addr_x, ValuesTypes::U32(1));

    let t1 = vec![rmw_x, rmw_x, rmw_x];
    let t2 = vec![rmw_x, rmw_x, rmw_x];

    let moves: Vec<Vec<EventInfo>> = vec![vec![], t1, t2];
    assert!(total_number_of_graphs(&moves) == 20);
}
#[test]
fn test_rmw_4() {
    // Simulates the program:
    // RMW(x) | RMW(x) | RMW(x)
    // Total number of graphs is the same as the total number of traces:
    // 3! = 6.

    let addr_x = Address::new(NonZeroUsize::new(1).unwrap());
    let rmw_x: EventInfo = mocked_rmw_read(addr_x, ValuesTypes::U32(1));

    let t1 = vec![rmw_x];
    let t2 = vec![rmw_x];
    let t3 = vec![rmw_x];

    let moves: Vec<Vec<EventInfo>> = vec![vec![], t1, t2, t3];
    assert!(total_number_of_graphs(&moves) == 6);
}
#[test]
fn test_rmw_5() {
    // Simulates the program:
    // RMW(x) | RMW(x) | RMW(x)
    // W(x)   | R(x)   | W(x)
    //        |        | RMW(x)
    // According to genmc, the total number of graphs is 210.

    let addr_x = Address::new(NonZeroUsize::new(1).unwrap());
    let rx = mocked_atomic_read(addr_x);
    let wx = mocked_atomic_write(addr_x, ValuesTypes::U32(1));
    let rmw_x: EventInfo = mocked_rmw_read(addr_x, ValuesTypes::U32(1));

    let t1 = vec![rmw_x, wx];
    let t2 = vec![rmw_x, rx];
    let t3 = vec![rmw_x, wx, rmw_x];

    let moves: Vec<Vec<EventInfo>> = vec![vec![], t1, t2, t3];
    assert!(total_number_of_graphs(&moves) == 210);
}

#[test]
fn test_rmw_6() {
    let addr_x = Address::new(NonZeroUsize::new(1).unwrap());
    let addr_y = Address::new(NonZeroUsize::new(2).unwrap());

    let wx = mocked_atomic_write(addr_x, ValuesTypes::U32(1));
    let wy = mocked_atomic_write(addr_y, ValuesTypes::U32(1));
    let rmw_x: EventInfo = mocked_rmw_read(addr_x, ValuesTypes::U32(1));

    let t1 = vec![rmw_x, wy];
    let t2 = vec![wx, rmw_x];
    let t3 = vec![wy];

    let moves: Vec<Vec<EventInfo>> = vec![vec![], t1, t2, t3];
    assert!(total_number_of_graphs(&moves) == 6);
}

#[test]
fn test_cas_0() {
    let addr_x = Address::new(NonZeroUsize::new(128).unwrap());

    let rx = mocked_atomic_read(addr_x);
    let cas_x_1: EventInfo = mocked_cas_read(
        addr_x,
        /* exp */ ValuesTypes::U32(0),
        /* desired */ ValuesTypes::U32(2),
    );

    let t1 = vec![cas_x_1, rx];
    let t2 = vec![rx, rx];

    let moves: Vec<Vec<EventInfo>> = vec![vec![], t1, t2];
    assert!(total_number_of_graphs(&moves) == 3);
}

#[test]
fn test_cas_1() {
    // Total number of graphs found by running genmc.
    let addr_x = Address::new(NonZeroUsize::new(4).unwrap());

    let cas_x_0_2: EventInfo = mocked_cas_read(
        addr_x,
        /* exp */ ValuesTypes::U32(0),
        /* desired */ ValuesTypes::U32(2),
    );
    let w_x_2 = mocked_atomic_write(addr_x, ValuesTypes::U32(2));

    let t1 = vec![w_x_2];
    let t2 = vec![cas_x_0_2];

    let moves: Vec<Vec<EventInfo>> = vec![vec![], t1, t2];
    assert!(total_number_of_graphs(&moves) == 2);
}

#[test]
fn test_cas_2() {
    let addr_x = Address::new(NonZeroUsize::new(128).unwrap());

    let cas_x_1: EventInfo = mocked_cas_read(
        addr_x,
        /* exp */ ValuesTypes::U32(0),
        /* desired */ ValuesTypes::U32(2),
    );
    let cas_x_2: EventInfo = mocked_cas_read(addr_x, ValuesTypes::U32(1), ValuesTypes::U32(2));

    let t1 = vec![cas_x_1];
    let t2 = vec![cas_x_2];

    let moves: Vec<Vec<EventInfo>> = vec![vec![], t1, t2];
    assert!(total_number_of_graphs(&moves) == 2);
}

#[test]
fn test_cas_3() {
    // Total number of graphs = number of traces = 4!/2/2 = 6.
    let addr_x = Address::new(NonZeroUsize::new(128).unwrap());

    let cas_x_1_2: EventInfo = mocked_cas_read(addr_x, ValuesTypes::U32(1), ValuesTypes::U32(2));
    let cas_x_0_2: EventInfo = mocked_cas_read(
        addr_x,
        /* exp */ ValuesTypes::U32(0),
        /* desired */ ValuesTypes::U32(2),
    );
    let w_x_1 = mocked_atomic_write(addr_x, ValuesTypes::U32(1));

    let t1 = vec![cas_x_0_2, cas_x_0_2];
    let t2 = vec![w_x_1, cas_x_1_2];

    let moves: Vec<Vec<EventInfo>> = vec![vec![], t1, t2];
    assert!(total_number_of_graphs(&moves) == 6);
}

#[test]
fn test_cas_4() {
    // Total number of graphs found by running genmc.
    let addr_x = Address::new(NonZeroUsize::new(128).unwrap());

    let cas_x_1_2: EventInfo = mocked_cas_read(addr_x, ValuesTypes::U32(1), ValuesTypes::U32(2));
    let cas_x_0_2: EventInfo = mocked_cas_read(
        addr_x,
        /* exp */ ValuesTypes::U32(0),
        /* desired */ ValuesTypes::U32(2),
    );
    let w_x_1 = mocked_atomic_write(addr_x, ValuesTypes::U32(1));
    let w_x_2 = mocked_atomic_write(addr_x, ValuesTypes::U32(2));

    let rx = mocked_atomic_read(addr_x);

    let t1 = vec![cas_x_0_2, cas_x_0_2, rx, w_x_2];
    let t2 = vec![w_x_1, cas_x_1_2, rx, w_x_1];

    let moves: Vec<Vec<EventInfo>> = vec![vec![], t1, t2];
    assert!(total_number_of_graphs(&moves) == 45);
}

#[test]
fn test_cas_5() {
    // Total number of graphs found by running genmc.
    let addr_x = Address::new(NonZeroUsize::new(4).unwrap());
    let cas_x_1_2: EventInfo = mocked_cas_read(addr_x, ValuesTypes::U32(1), ValuesTypes::U32(2));
    let cas_x_0_2: EventInfo = mocked_cas_read(
        addr_x,
        /* exp */ ValuesTypes::U32(0),
        /* desired */ ValuesTypes::U32(2),
    );
    let cas_x_2_0: EventInfo = mocked_cas_read(
        addr_x,
        /* exp */ ValuesTypes::U32(2),
        /* desired */ ValuesTypes::U32(0),
    );
    let cas_x_1_0: EventInfo = mocked_cas_read(
        addr_x,
        /* exp */ ValuesTypes::U32(1),
        /* desired */ ValuesTypes::U32(0),
    );
    let cas_x_2_1: EventInfo = mocked_cas_read(
        addr_x,
        /* exp */ ValuesTypes::U32(2),
        /* desired */ ValuesTypes::U32(1),
    );
    let w_x_1 = mocked_atomic_write(addr_x, ValuesTypes::U32(1));
    let w_x_2 = mocked_atomic_write(addr_x, ValuesTypes::U32(2));
    let rx = mocked_atomic_read(addr_x);

    let t1 = vec![cas_x_0_2, cas_x_1_0, w_x_2];
    let t2 = vec![cas_x_1_2, cas_x_1_0, w_x_1];
    let t3 = vec![cas_x_2_0, cas_x_2_1, rx];

    let moves: Vec<Vec<EventInfo>> = vec![vec![], t1, t2, t3];
    // TODO: test this number!
    assert!(total_number_of_graphs(&moves) == 662);
}

#[test]
fn test_cas_6() {
    // Total number of graphs found by running genmc.
    let addr_x = Address::new(NonZeroUsize::new(4).unwrap());
    let cas_x_0_2: EventInfo = mocked_cas_read(
        addr_x,
        /* exp */ ValuesTypes::U32(0),
        /* desired */ ValuesTypes::U32(2),
    );
    let cas_x_2_1: EventInfo = mocked_cas_read(
        addr_x,
        /* exp */ ValuesTypes::U32(2),
        /* desired */ ValuesTypes::U32(1),
    );
    let w_x_0 = mocked_atomic_write(addr_x, ValuesTypes::U32(0));

    let t1 = vec![cas_x_0_2, cas_x_2_1, w_x_0];
    let t2 = vec![cas_x_0_2, cas_x_2_1, w_x_0];
    let t3 = vec![cas_x_0_2, cas_x_2_1, w_x_0];

    let moves: Vec<Vec<EventInfo>> = vec![vec![], t1, t2, t3];
    assert!(total_number_of_graphs(&moves) == 912);
}

#[test]
fn test_cas_7() {
    let addr_x = Address::new(NonZeroUsize::new(4).unwrap());
    let cas_x_0_2: EventInfo = mocked_cas_read(
        addr_x,
        /* exp */ ValuesTypes::U32(0),
        /* desired */ ValuesTypes::U32(2),
    );
    let t = [cas_x_0_2];

    let mut moves: Vec<Vec<EventInfo>> = vec![vec![]];
    let n = 10;
    for _ in 1..=n {
        moves.push(t.to_vec());
    }
    // As only one event is a RMW and the rest acts like a read,
    // once we fix the thread that is the RMW all the others
    // have only one choice.
    assert!(total_number_of_graphs(&moves) == n);
}

#[test]
fn test_rmw_7() {
    // Total number of graphs found by running genmc.
    let addr_x = Address::new(NonZeroUsize::new(4).unwrap());
    let rmw_x = mocked_rmw_read(addr_x, ValuesTypes::U32(1));

    let t1 = vec![rmw_x];
    let t2 = vec![rmw_x];
    let t3 = vec![rmw_x];
    let t4 = vec![rmw_x];
    let t5 = vec![rmw_x];

    let moves: Vec<Vec<EventInfo>> = vec![vec![], t1, t2, t3, t4, t5];
    // TODO: test this number!
    assert!(total_number_of_graphs(&moves) == 120);
}

#[test]
fn test_cas_big_0() {
    // Total number of graphs found by running genmc.
    let addr_x = Address::new(NonZeroUsize::new(4).unwrap());
    let addr_y = Address::new(NonZeroUsize::new(8).unwrap());
    let cas_x_1_2: EventInfo = mocked_cas_read(addr_x, ValuesTypes::U32(1), ValuesTypes::U32(2));
    let cas_x_0_2: EventInfo = mocked_cas_read(
        addr_x,
        /* exp */ ValuesTypes::U32(0),
        /* desired */ ValuesTypes::U32(2),
    );
    let cas_y_2_0: EventInfo = mocked_cas_read(
        addr_y,
        /* exp */ ValuesTypes::U32(2),
        /* desired */ ValuesTypes::U32(0),
    );

    let w_x_1 = mocked_atomic_write(addr_x, ValuesTypes::U32(1));
    let w_x_2 = mocked_atomic_write(addr_x, ValuesTypes::U32(2));
    let w_y_1 = mocked_atomic_write(addr_y, ValuesTypes::U32(1));
    let w_y_2 = mocked_atomic_write(addr_y, ValuesTypes::U32(2));

    let rx = mocked_atomic_read(addr_x);
    let ry = mocked_atomic_read(addr_y);

    let t1 = vec![cas_x_0_2, cas_x_0_2, rx, w_x_2];
    let t2 = vec![w_x_1, cas_x_1_2, rx, w_x_1, w_y_2];
    let t3 = vec![w_y_1, cas_y_2_0, rx, ry];

    let moves: Vec<Vec<EventInfo>> = vec![vec![], t1, t2, t3];
    assert!(total_number_of_graphs(&moves) == 624);
}

#[test]
fn test_cas_big_1() {
    // Total number of graphs found by running genmc.
    let addr_x = Address::new(NonZeroUsize::new(4).unwrap());
    let cas_x_1_2: EventInfo = mocked_cas_read(addr_x, ValuesTypes::U32(1), ValuesTypes::U32(2));
    let cas_x_0_2: EventInfo = mocked_cas_read(
        addr_x,
        /* exp */ ValuesTypes::U32(0),
        /* desired */ ValuesTypes::U32(2),
    );
    let cas_x_1_0: EventInfo = mocked_cas_read(
        addr_x,
        /* exp */ ValuesTypes::U32(1),
        /* desired */ ValuesTypes::U32(0),
    );
    let cas_x_2_1: EventInfo = mocked_cas_read(
        addr_x,
        /* exp */ ValuesTypes::U32(2),
        /* desired */ ValuesTypes::U32(1),
    );
    let w_x_0 = mocked_atomic_write(addr_x, ValuesTypes::U32(0));
    let w_x_1 = mocked_atomic_write(addr_x, ValuesTypes::U32(1));
    let w_x_2 = mocked_atomic_write(addr_x, ValuesTypes::U32(2));

    let rx = mocked_atomic_read(addr_x);

    let t1 = vec![w_x_1, cas_x_1_0, rx, cas_x_0_2];
    let t2 = vec![w_x_2, cas_x_2_1, cas_x_1_2];
    let t3 = vec![cas_x_2_1, cas_x_0_2, w_x_0, rx];

    let moves: Vec<Vec<EventInfo>> = vec![vec![], t1, t2, t3];
    assert!(total_number_of_graphs(&moves) == 4892);
}

#[test]
fn test_rmw_big_0() {
    // According to genmc, the total number of graphs is 1092.

    let addr_x = Address::new(NonZeroUsize::new(128).unwrap());
    let rx = mocked_atomic_read(addr_x);
    let wx = mocked_atomic_write(addr_x, ValuesTypes::U32(1));
    let rmw_x: EventInfo = mocked_rmw_read(addr_x, ValuesTypes::U32(1));

    let t1 = vec![rmw_x, wx, rx, rmw_x];
    let t2 = vec![rmw_x, wx, rx];
    let t3 = vec![rmw_x, wx];

    let moves: Vec<Vec<EventInfo>> = vec![vec![], t1, t2, t3];
    assert!(total_number_of_graphs(&moves) == 1092);
}

#[test]
fn test_rmw_big_1() {
    // Total number of graphs found in genmc: 29.

    let addr_x = Address::new(NonZeroUsize::new(128).unwrap());
    let rmw_x: EventInfo = mocked_rmw_read(addr_x, ValuesTypes::U32(1));
    let addr_y = Address::new(NonZeroUsize::new(2).unwrap());
    let rmw_y: EventInfo = mocked_rmw_read(addr_y, ValuesTypes::U32(1));

    let t1 = vec![rmw_x, rmw_x, rmw_y, rmw_y, rmw_y, rmw_y];
    let t2 = vec![rmw_y, rmw_y, rmw_x, rmw_x, rmw_x, rmw_x];

    let moves: Vec<Vec<EventInfo>> = vec![vec![], t1, t2];
    assert!(total_number_of_graphs(&moves) == 29);
}

#[test]
fn test_rmw_big_2() {
    // Total number of graphs found in genmc: 1400.

    let addr_x = Address::new(NonZeroUsize::new(128).unwrap());
    let rmw_x: EventInfo = mocked_rmw_read(addr_x, ValuesTypes::U32(1));
    let rx = mocked_atomic_read(addr_x);
    let wx = mocked_atomic_write(addr_x, ValuesTypes::U32(1));

    let t1 = vec![rx, wx];
    let t2 = vec![rmw_x];
    let t3 = vec![rmw_x, rmw_x, rmw_x];
    let t4 = vec![rx, wx];

    let moves: Vec<Vec<EventInfo>> = vec![vec![], t1, t2, t3, t4];
    assert!(total_number_of_graphs(&moves) == 1400);
}

#[test]
fn test_rmw_big_3() {
    // Total number of graphs is equal to the number of traces:
    // 8!/3!/2!/2! = 1680.

    let addr_x = Address::new(NonZeroUsize::new(128).unwrap());
    let rmw_x: EventInfo = mocked_rmw_read(addr_x, ValuesTypes::U32(1));

    let t1 = vec![rmw_x, rmw_x];
    let t2 = vec![rmw_x, rmw_x, rmw_x];
    let t3 = vec![rmw_x, rmw_x];
    let t4 = vec![rmw_x];

    let moves: Vec<Vec<EventInfo>> = vec![vec![], t1, t2, t3, t4];
    assert!(total_number_of_graphs(&moves) == 1680);
}

#[test]
fn test_multiple_local_variable_changes() {
    let addr_x = Address::new(NonZeroUsize::new(128).unwrap());
    let addr_y = Address::new(NonZeroUsize::new(2).unwrap());
    let rx = mocked_read(addr_x);
    let ry = mocked_read(addr_y);

    let wx = mocked_write(addr_x);
    let wy = mocked_write(addr_y);

    let mut t1 = Vec::new();
    let mut t2 = Vec::new();

    for _ in 0..1000 {
        t1.push(rx);
        t1.push(wx);
        t2.push(ry);
        t2.push(wy);
    }

    let _moves: Vec<Vec<EventInfo>> = vec![vec![], t1];
    // We dont have support for this yet. I think the simplest solution would be
    // to keep track of every write/read to a address per thread.
    // This way, with some smart `breaks` we could for each read/write check only
    // edges for the latest event in its thread (which is the only valid edge),
    // thus reducing complexity from O(N^2) to O(1) :D
    // It is also necessary to have a faster check of adding rf and co edges.
    // The current idea of adding the event, checking for consistency, and then
    // removing makes it O(N^3).
    // assert!(total_number_of_graphs(&_moves) == 1);
}

struct NextEventAux {
    use_moves: bool,
    tid: usize,
    ev_info: Option<EventInfo>,
}

impl NextEventAux {
    pub fn new(use_moves: bool, tid: usize, ev_info: Option<EventInfo>) -> Self {
        Self {
            use_moves,
            tid,
            ev_info,
        }
    }
}

#[allow(dead_code)]
fn total_number_of_graphs(moves: &Vec<Vec<EventInfo>>) -> u32 {
    let mut ptr = vec![0; moves.len()];
    let mut trust = TruST::new();
    trust.test_mode = true;
    let end_event = EventInfo::end();
    let mut total_number_of_graphs = 0;
    loop {
        let res = get_next_event(&mut trust, moves, &ptr);
        let ev_info = if !res.use_moves {
            if res.tid == 0 {
                // full execution graph explored
                total_number_of_graphs += 1;
                end_event
            } else {
                res.ev_info.unwrap()
            }
        } else {
            let tid = res.tid;
            moves[tid][ptr[tid]]
        };
        let tid = res.tid;
        trace!("before action {:?} {:?}", tid, ev_info);
        // For tests we initialize everything with 0
        if let Some(addr) = ev_info.addr {
            trust.update_init_value_if_not_set(addr, ValuesTypes::U32(0));
        }

        let action = trust.handle_new_event(tid, ev_info);
        trace!(
            "({:?}, {:?}) = {:?}, {:?}",
            tid,
            ptr[tid],
            ev_info.event_type,
            action
        );

        match action {
            TrustAction::Add => {
                if res.use_moves {
                    ptr[tid] += 1;
                } else {
                    ptr[0] += 1; // include in total number of moves only
                }
            }
            TrustAction::Rebuild => {
                // again from the start
                trace!("Time to rebuild");
                // trust.print_reconstruction_trace();
                ptr = vec![0; moves.len()];
            }
            TrustAction::End => {
                // every graph was explored
                break;
            }
        }
    }
    dbg!(total_number_of_graphs);
    total_number_of_graphs
}

// Gets the next event given the current state,
// a vector with all the available moves for each thread and a
// vector with a pointer of the next move for each thread.
fn get_next_event(
    trust: &mut TruST,
    moves: &Vec<Vec<EventInfo>>,
    ptr: &Vec<usize>,
) -> NextEventAux {
    if trust.is_rebuilding() {
        // must return the next event in the trace
        let (cur_tid, cur_ev_info) = trust.get_next_trace_event();
        let cur_tid = u64::from(cur_tid) as usize;
        if cur_ev_info.is_rmw && cur_ev_info.event_type == EventType::Write {
            // not include in moves
            let addr = trust
                .virtual_address_mapper
                .get_real_address(&cur_ev_info.addr.unwrap());
            return NextEventAux::new(
                false,
                cur_tid,
                Some(mocked_rmw_write(addr, cur_ev_info.val)),
            );
        }
        return NextEventAux::new(true, cur_tid, None);
    }
    // If there is a rmw we are forced to make it the next operation.
    if trust.expects_rmw_write() {
        let (tid, addr, _lenght, val) = trust.get_next_rmw_write();
        let tid = u64::from(tid) as usize;
        let res = NextEventAux::new(false, tid, Some(mocked_rmw_write(addr, val)));
        return res;
    }
    // returns from the first thread that still has events
    for tid in 1..moves.len() {
        if ptr[tid] < moves[tid].len() {
            return NextEventAux::new(true, tid, None);
        }
    }
    // end of events
    NextEventAux::new(false, 0, None)
}

#[test]
fn test_serialization() {
    use std::num::NonZeroUsize;
    let addr_x = Address::new(NonZeroUsize::new(128).unwrap());
    let addr_y = Address::new(NonZeroUsize::new(2).unwrap());
    let rx = mocked_read(addr_x);
    let ry = mocked_read(addr_y);

    let wx = mocked_write(addr_x);
    let wy = mocked_write(addr_y);

    let mut trust = TruST::new();
    let action = trust.handle_new_event(/* tid */ 1, rx);
    assert!(action == TrustAction::Add);
    test_serialize_deserialize(&trust);
    trust.handle_new_event(/* tid */ 1, wx);
    assert!(action == TrustAction::Add);
    test_serialize_deserialize(&trust);
    trust.handle_new_event(/* tid */ 2, ry);
    assert!(action == TrustAction::Add);
    test_serialize_deserialize(&trust);
    trust.handle_new_event(/* tid */ 2, wy);
    assert!(action == TrustAction::Add);
    test_serialize_deserialize(&trust);
    dbg!(&trust.serialize());
}

fn test_serialize_deserialize(trust: &TruST) {
    let new_trust = TruST::deserialize(trust.serialize());
    assert!(trust_object_is_eq(trust, &new_trust));
    assert!(new_trust.virtual_address_mapper.is_empty());
    assert!(new_trust.state.graph.is_empty());
}

fn trust_object_is_eq(trust_1: &TruST, trust_2: &TruST) -> bool {
    trust_1.total_graphs == trust_2.total_graphs
        && trust_1.virtual_address_mapper.get_virtual_address_cnt()
            == trust_2.virtual_address_mapper.get_virtual_address_cnt()
        && trust_1.state.graph.get_id_count() == trust_2.state.graph.get_id_count()
        && trust_1.state.stack == trust_2.state.stack
        && trust_1.state.rebuild_trace == trust_2.state.rebuild_trace
        && trust_1.cached_address_info == trust_2.cached_address_info
}
