use lotto::raw;
use lotto::sync::HandlerWrapper;
use lotto::Stateful;
use lotto::{
    base::{Category, Value},
    brokers::statemgr::*,
    cli::flags::{FlagKey, STR_CONVERTER_BOOL},
    collections::FxHashMap,
    engine::handler::{self, TaskId},
    log::*,
};

use std::collections::BTreeMap;
use std::marker::PhantomData;
use std::sync::atomic::{AtomicBool, Ordering};

use crate::handlers::cas;
use crate::handlers::stacktrace;
use crate::idmap::IdMap;
use crate::Eff;
use crate::{Event, GenericEventCore, GenericEventCoreRef, StackTrace, Transition};

pub static HANDLER: HandlerWrapper<EventHandler> = HandlerWrapper::new(|| EventHandler {
    cfg: Config {
        enabled: AtomicBool::new(false),
    },
    pers: Persistent {
        tasks: BTreeMap::new(),
    },
    pc_cnt: FxHashMap::default(),
    st_map: IdMap::default(),
});

/// The index in the `st_map`.
type StackTraceId = usize;

type EventCore = GenericEventCore<StackTraceId>;

pub struct Count {
    capture: u64,
    resume: u64,
}

#[derive(Stateful)]
pub struct EventHandler {
    #[config]
    pub cfg: Config,
    #[persistent]
    pub pers: Persistent,

    pub pc_cnt: FxHashMap<EventCore, Count>,

    // Do not store stacktrace in pc_cnt to save some RAM.
    pub st_map: IdMap<StackTrace>,
}

impl handler::Handler for EventHandler {
    fn handle(&mut self, ctx: &raw::context_t, e: &mut raw::event_t) {
        if !self.cfg.enabled.load(Ordering::Relaxed) {
            return;
        }
        let id = TaskId::new(ctx.id);
        let stacktrace = stacktrace::get_task_stacktrace(id).unwrap_or_default();
        let ma = cas::get_rt_memory_access(id);
        let stid = self.st_map.put(&stacktrace);
        let transition = Transition::new(ctx);

        // During capture, we are going to check whether this event
        // should be blocked, if the operation were to read this
        // value. That is, the really executed event might not read
        // this value, and we must update the real value after it
        // really happens. See posthandle.
        let eff = Eff::from(ma.as_ref());

        let ecore = EventCore {
            t: transition.clone(),
            _phantom: PhantomData,
            stacktrace: stid,
            eff,
        };
        let cnt = capture(&mut self.pc_cnt, ecore);
        let event = Event {
            t: transition,
            clk: e.clk,
            cnt: cnt.capture,
            stacktrace,
            m: ma,
        };
        //println!("updated event {} capture -> {}", event.t, cnt.capture);
        self.pers.tasks.insert(id, event);
    }

    fn posthandle(&mut self, ctx: &raw::context_t) {
        if ctx.cat == Category::CAT_NONE || !self.cfg.enabled.load(Ordering::Relaxed) {
            return;
        }
        let entry = self
            .pers
            .tasks
            .get_mut(&TaskId(ctx.id))
            .expect("handler_event's posthandle expects event data from capture");
        let mut ecore = ecore_from_event(&mut self.st_map, &*entry);

        let Some(ref oldm) = entry.m else {
            // Not memory operation
            resume(&mut self.pc_cnt, ecore);
            return;
        };

        // Task i just executed a memory operation, and we should
        // update the value to reflect the "real" value.
        let Some(ref realm) = cas::get_rt_memory_access(TaskId(ctx.id)) else {
            panic!("real memory access not available");
        };
        let oldv = oldm.loaded_value();
        let realv = realm.loaded_value();
        if oldv == realv {
            // Prediction is correct.
            resume(&mut self.pc_cnt, ecore);
            return;
        }
        let oldeff = Eff::from(oldm);
        let neweff = Eff::from(realm);

        // The actual event reads another value. Uncapture the oldeff,
        // capture neweff, and resume neweff.
        ecore.eff = oldeff;
        self.pc_cnt
            .entry(ecore.clone())
            .and_modify(|cnt| cnt.capture -= 1);
        ecore.eff = neweff;
        let newcnt = self
            .pc_cnt
            .entry(ecore)
            .and_modify(|cnt| {
                cnt.capture += 1;
                cnt.resume = cnt.capture;
            })
            .or_insert(Count {
                capture: 1,
                resume: 1,
            });

        // Update the record.
        entry.cnt = newcnt.resume;
        entry.m = Some(realm.clone());
        //println!("RESUME event {} cnt={}", entry.t, newcnt.resume);
    }
}

fn ecore_from_event(st_map: &mut IdMap<StackTrace>, e: &Event) -> EventCore {
    let stid = st_map.put(&e.stacktrace);
    EventCore {
        t: e.t.clone(),
        stacktrace: stid,
        eff: Eff::from(e.m.as_ref()),
        _phantom: PhantomData,
    }
}

fn capture(pc_cnt: &mut FxHashMap<EventCore, Count>, ecore: EventCore) -> &mut Count {
    pc_cnt
        .entry(ecore)
        .and_modify(|cnt| cnt.capture = cnt.resume + 1)
        .or_insert(Count {
            capture: 1,
            resume: 0,
        })
}

fn resume(pc_cnt: &mut FxHashMap<EventCore, Count>, ecore: EventCore) -> &mut Count {
    pc_cnt
        .entry(ecore)
        .and_modify(|cnt| cnt.resume = cnt.capture)
        .or_insert(Count {
            capture: 0,
            resume: 1,
        })
}

//
// States
//

#[derive(Encode, Decode, Debug)]
pub struct Config {
    pub enabled: AtomicBool,
}

impl Marshable for Config {
    fn print(&self) {
        info!("enabled = {}", self.enabled.load(Ordering::Relaxed));
    }
}

#[derive(Encode, Decode, Debug)]
pub struct Persistent {
    pub tasks: BTreeMap<TaskId, Event>,
}

impl Marshable for Persistent {
    fn print(&self) {
        for (id, info) in self.tasks.iter() {
            info!("  - task {} : {}", id, info);
        }
    }
}

//
// Flags
//

pub static FLAG_HANDLER_EVENT_ENABLED: FlagKey = FlagKey::new(
    c"FLAG_HANDLER_EVENT_ENABLED",
    c"",
    c"handler-event",
    c"",
    c"enable the event handler",
    Value::Bool(false),
    &STR_CONVERTER_BOOL,
    Some(_handler_event_enabled_callback),
);

unsafe extern "C" fn _handler_event_enabled_callback(v: raw::value, _: *mut std::ffi::c_void) {
    let enabled = Value::from(v).is_on();
    HANDLER.cfg.enabled.store(enabled, Ordering::Relaxed);
}

//
// Registration
//
pub fn register() {
    lotto::engine::handler::register(&*HANDLER);
    lotto::brokers::statemgr::register(&*HANDLER);
}

pub fn register_flags() {
    let _ = FLAG_HANDLER_EVENT_ENABLED.get();
}

//
// Interfaces
//

/// Return how many times this event actually happened.
pub fn get_event_happened_cnt(ecore_ref: GenericEventCoreRef<StackTrace>) -> u64 {
    let handler = unsafe { HANDLER.get_mut() };
    let ecore = GenericEventCore::from_ref(ecore_ref, |st| handler.st_map.put(&st));
    handler
        .pc_cnt
        .get(&ecore)
        .map(|cnt| cnt.resume)
        .unwrap_or(0)
}

pub fn get_event(task: TaskId) -> Option<Event> {
    HANDLER.pers.tasks.get(&task).cloned()
}

/// Get real-time event.
///
/// Call [`update_event`] if you need to access the "current" memory
/// value.
pub fn get_rt_event(task: TaskId) -> Option<Event> {
    HANDLER.pers.tasks.get(&task).cloned().map(|mut event| {
        if event.m.as_ref().is_some_and(|m| m.has_invalid_real_addr()) {
            event.m = cas::get_rt_memory_access(task);
            assert!(event.m.is_some(), "RT event should have RT modify");
        }
        event
    })
}

/// Update the event record (both persistent and rt). This will in
/// turn update the [`MemoryAccess`] records.
pub fn update_event(task: TaskId) {
    cas::update_memory_access(task);
    let pers = unsafe { &mut *HANDLER.persistent_mut() };
    if let Some(e) = pers.tasks.get_mut(&task) {
        e.m = cas::get_memory_access(task);
    }
}
