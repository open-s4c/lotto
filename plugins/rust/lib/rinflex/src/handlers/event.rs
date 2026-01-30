use lotto::raw;
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
use std::sync::LazyLock;

use crate::handlers::cas;
use crate::handlers::stacktrace;
use crate::idmap::IdMap;
use crate::Eff;
use crate::{Event, GenericEventCore, StackTrace, Transition};

pub static HANDLER: LazyLock<EventHandler> = LazyLock::new(|| EventHandler {
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

#[derive(Stateful)]
pub struct EventHandler {
    #[config]
    pub cfg: Config,
    #[persistent]
    pub pers: Persistent,

    pub pc_cnt: FxHashMap<GenericEventCore<StackTraceId>, u64>,

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

        let ecore = GenericEventCore {
            t: transition.clone(),
            _phantom: PhantomData,
            stacktrace: stid,
            eff,
            // addr: ma.as_ref().map(|ma| ma.addr().to_owned()),
            // rval: ma.as_ref().map(MemoryAccess::loaded_value).flatten(),
        };
        let cnt = *self
            .pc_cnt
            .entry(ecore)
            .and_modify(|c| *c += 1)
            .or_insert(1);
        let event = Event {
            t: transition,
            clk: e.clk,
            cnt,
            stacktrace,
            m: ma,
        };
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

        // Task i just executed a memory operation, and we should
        // update the value to reflect the "real" value.
        let Some(ref oldm) = entry.m else {
            return;
        };
        let Some(ref realm) = cas::get_rt_memory_access(TaskId(ctx.id)) else {
            panic!("real memory access not available");
        };
        let oldv = oldm.loaded_value();
        let realv = realm.loaded_value();
        if oldv == realv {
            return;
        }
        let oldeff = Eff::from(oldm);
        let neweff = Eff::from(realm);

        // The actual event reads another value. Update counters.
        let stid = self.st_map.put(&entry.stacktrace);
        let mut ecore = GenericEventCore {
            _phantom: PhantomData,
            t: entry.t.clone(),
            stacktrace: stid,
            eff: oldeff,
        };
        self.pc_cnt.entry(ecore.clone()).and_modify(|c| *c -= 1);
        ecore.eff = neweff;
        let newcnt = *self
            .pc_cnt
            .entry(ecore)
            .and_modify(|c| *c += 1)
            .or_insert(1);

        // Update the record.
        entry.cnt = newcnt;
        entry.m = Some(realm.clone());
    }
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
