#![cfg(feature = "runtime")]

extern crate lotto_link;

use lotto::base::{CapturePoint, EventType, StableAddress, StableAddressMethod, TaskId};
use lotto::{brokers::statemgr::Stateful, engine::handler::Handler, raw};
use rinflex::{
    handlers::{
        event,
        order_enforcer::{Final, OrderEnforcer, HANDLER},
    },
    num::U64OrInf,
    *,
};

#[test]
fn blocked_constraints_track_sources_and_unblock_by_id() {
    let capture = |handler: &mut OrderEnforcer, id, type_id, runnable: &[TaskId]| {
        let ctx = raw::capture_point {
            id,
            type_id: type_id as u16,
            ..unsafe { std::mem::zeroed() }
        };
        let mut cappt: raw::event_t = unsafe { std::mem::zeroed() };
        unsafe { raw::tidset_init(&mut cappt.tset) };
        for tid in runnable {
            unsafe { raw::tidset_insert(&mut cappt.tset, tid.0) };
        }
        handler.handle(unsafe { CapturePoint::wrap(&ctx) }, &mut cappt);
        unsafe { raw::tidset_fini(&mut cappt.tset) };
    };
    let source = Event {
        t: Transition {
            id: TaskId(2),
            pc: StableAddress::with_method(0x100, StableAddressMethod::STABLE_ADDRESS_METHOD_MASK),
            type_id: EventType::from_raw_value(raw::EVENT_MA_WRITE),
            after: false,
        },
        clk: 1,
        cnt: 1,
        stacktrace: StackTrace::default(),
        m: None,
    };
    let mut target = source.clone();
    target.t.id = TaskId(3);
    target.t.type_id = EventType::from_raw_value(raw::EVENT_MA_READ);
    let constraint = Constraint {
        c: PrimitiveConstraint {
            source,
            target: target.clone(),
            clk: 0,
        },
        virt: false,
        positive: true,
        id: 17,
    };
    let cases = [
        ("target absent", false, true, 1, 1, 2, false),
        ("target blocked", true, true, 1, 1, 2, true),
        ("other blocked event", true, false, 1, 1, 2, false),
        ("different target occurrence", true, true, 1, 2, 2, false),
        ("source already occurred", true, true, 0, 1, 2, false),
        ("another thread exits", true, true, 1, 1, 4, false),
    ];
    for (name, blocked, matches, source_count, target_count, exiting, expected) in cases {
        let mut current = target.clone();
        if !matches {
            current.t.type_id = EventType::from_raw_value(raw::EVENT_MA_WRITE);
        }
        unsafe { event::HANDLER.persistent_mut() }
            .tasks
            .insert(target.t.id, current.clone());
        let mut c = constraint.clone();
        c.c.source.cnt = source_count;
        c.c.target.cnt = target_count;
        let mut other = constraint.clone();
        other.id = 42;
        other.c.source.t.id = TaskId(9);
        other.c.target = current;
        let handler = unsafe { HANDLER.get_mut() };
        handler.fin = Final {
            constraints: vec![c, other],
            should_discard: false,
        };
        handler.block.clear();
        handler.shutdown = false;
        handler.max_clock = U64OrInf::inf();
        if blocked {
            capture(handler, 1, raw::EVENT_MA_READ, &[target.t.id]);
            assert!(handler.block.contains_key(&target.t.id));
        }
        capture(handler, exiting, raw::EVENT_TASK_FINI, &[]);
        assert_eq!(handler.fin.should_discard, expected, "{name}");
    }
    let mut first = constraint.clone();
    first.c.source.cnt = 2;
    let mut second = constraint.clone();
    second.id = 42;
    second.c.source.t.id = TaskId(4);
    let mut unrelated = constraint.clone();
    unrelated.id = 99;
    unrelated.c.source.t.id = TaskId(5);
    unrelated.c.target.t.type_id = EventType::from_raw_value(raw::EVENT_MA_WRITE);
    let handler = unsafe { HANDLER.get_mut() };
    handler.fin = Final {
        constraints: vec![first, second, unrelated],
        should_discard: false,
    };
    handler.block.clear();
    handler.shutdown = false;
    unsafe { event::HANDLER.persistent_mut() }
        .tasks
        .insert(target.t.id, target.clone());
    capture(handler, 1, raw::EVENT_MA_READ, &[target.t.id]);
    assert_eq!(
        handler.block[&target.t.id],
        vec![(17, TaskId(2)), (42, TaskId(4))]
    );
    capture(handler, 1, raw::EVENT_MA_READ, &[target.t.id]);
    assert_eq!(handler.block[&target.t.id].len(), 2);

    for (index, remaining) in [(2, 2), (0, 2), (0, 1), (1, 0)] {
        let source = handler.fin.constraints[index].c.source.clone();
        let ctx = raw::capture_point {
            id: source.t.id.0,
            type_id: raw::EVENT_MA_WRITE as u16,
            ..unsafe { std::mem::zeroed() }
        };
        unsafe { event::HANDLER.persistent_mut() }
            .tasks
            .insert(source.t.id, source);
        handler.posthandle(unsafe { CapturePoint::wrap(&ctx) });
        assert_eq!(
            handler.block.get(&target.t.id).map_or(0, Vec::len),
            remaining
        );
        if remaining == 1 {
            assert_eq!(handler.block[&target.t.id], vec![(42, TaskId(4))]);
            capture(handler, 2, raw::EVENT_TASK_FINI, &[]);
            assert!(!handler.fin.should_discard);
        }
    }
    assert!(handler.block.is_empty());
    unsafe { event::HANDLER.persistent_mut() }.tasks.clear();
}
