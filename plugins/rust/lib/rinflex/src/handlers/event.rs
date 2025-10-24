use lotto::collections::FxHashMap;
use lotto::{base::Category, base::StableAddress, raw, Stateful};
use lotto::{
    base::Value,
    brokers::statemgr::*,
    cli::flags::{FlagKey, STR_CONVERTER_BOOL},
    engine::handler::{self, TaskId},
    log::*,
};
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::LazyLock;

use crate::handlers::cas;
use crate::handlers::stacktrace;
use crate::idmap::IdMap;
use crate::{Event, StackTrace};

pub static HANDLER: LazyLock<EventHandler> = LazyLock::new(|| EventHandler {
    cfg: Config {
        enabled: AtomicBool::new(false),
    },
    pers: Persistent {
        tasks: FxHashMap::default(),
    },
    pc_cnt: FxHashMap::default(),
    st_map: IdMap::default(),
});

#[derive(Stateful)]
pub struct EventHandler {
    #[config]
    pub cfg: Config,
    #[persistent]
    pub pers: Persistent,

    pub pc_cnt: FxHashMap<(TaskId, StableAddress, Category, usize), u64>,

    // Do not store stacktrace in pc_cnt to save some RAM.
    pub st_map: IdMap<StackTrace>,
}

impl handler::Handler for EventHandler {
    fn handle(&mut self, ctx: &raw::context_t, event: &mut raw::event_t) {
        if !self.cfg.enabled.load(Ordering::Relaxed) {
            return;
        }
        let id = TaskId::new(ctx.id);
        let stacktrace = stacktrace::get_task_stacktrace(id).unwrap_or(Vec::new());
        let stid = self.st_map.put(&stacktrace);
        let mut event = Event::new(ctx, event, stacktrace.clone());
        let pc = event.t.pc.clone();
        let cnt = *self
            .pc_cnt
            .entry((id, pc, ctx.cat, stid))
            .and_modify(|c| *c += 1)
            .or_insert(1);
        event.cnt = cnt;
        event.m = cas::get_modify(id);
        self.pers.tasks.insert(id, event);
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
    pub tasks: FxHashMap<TaskId, Event>,
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
pub fn get_rt_event(task: TaskId) -> Option<Event> {
    HANDLER.pers.tasks.get(&task).cloned().map(|mut event| {
        if event.m.as_ref().is_some_and(|m| m.has_invalid_real_addr()) {
            event.m = cas::get_rt_modify(task);
            assert!(event.m.is_some(), "RT event should have RT modify");
        }
        event
    })
}
