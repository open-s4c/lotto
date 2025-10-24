//! # CAS predictor
//!
//! This handler records information to predict whether a CAS
//! operation can succeed or not.

use lotto::collections::FxHashMap;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::LazyLock;

use crate::modify::{Modify, ModifyKind, VAddr};
use bincode::{Decode, Encode};
use lotto::base::category::Category;
use lotto::base::{HandlerArg, Value};
use lotto::brokers::Marshable;
use lotto::cli::flags::{FlagKey, STR_CONVERTER_BOOL};
use lotto::engine::handler::{Handler, TaskId};
use lotto::log::*;
use lotto::raw;
use lotto::Stateful;

static HANDLER: LazyLock<CasPredictor> = LazyLock::new(|| CasPredictor {
    cfg: Config {
        enabled: AtomicBool::new(false),
    },
    pers: Persistent {
        tasks: FxHashMap::default(),
    },
    rt_map: FxHashMap::default(),
});

#[derive(Stateful)]
struct CasPredictor {
    #[config]
    cfg: Config,
    #[persistent]
    pers: Persistent,

    // Real-time values from the current execution.
    rt_map: FxHashMap<TaskId, Modify>,
}

//
// Handler
//

impl Handler for CasPredictor {
    fn handle(&mut self, ctx: &raw::context_t, _event: &mut raw::event_t) {
        if !self.cfg.enabled.load(Ordering::Relaxed) {
            return;
        }
        let kind: Option<ModifyKind> = match ctx.cat {
            // CAS
            Category::CAT_BEFORE_CMPXCHG
            | Category::CAT_AFTER_CMPXCHG_S
            | Category::CAT_AFTER_CMPXCHG_F => Some(ModifyKind::Cas {
                expected: get_val(ctx.args[2]),
                new: get_val(ctx.args[3]),
            }),

            // RMW
            Category::CAT_BEFORE_RMW | Category::CAT_AFTER_RMW => Some(ModifyKind::Rmw {
                value: get_val(ctx.args[2]),
            }),

            // XCHG
            Category::CAT_BEFORE_XCHG | Category::CAT_AFTER_XCHG => Some(ModifyKind::Xchg {
                value: get_val(ctx.args[2]),
            }),

            // Plain writes
            Category::CAT_BEFORE_WRITE => Some(ModifyKind::Write),

            // Atomic writes
            Category::CAT_BEFORE_AWRITE | Category::CAT_AFTER_AWRITE => Some(ModifyKind::AWrite {
                value: get_val(ctx.args[2]),
            }),

            // Ignore other non-modifying operations
            _ => None,
        };

        if let Some(kind) = kind {
            let raw_addr = get_addr(ctx.args[0]);
            let addr = VAddr::get(ctx, raw_addr);
            let size = get_val(ctx.args[1]);
            let item = Modify {
                addr,
                real_addr: raw_addr as u64,
                size: size as u8,
                kind,
            };
            self.rt_map.insert(TaskId::new(ctx.id), item.clone());
            self.pers.tasks.insert(TaskId::new(ctx.id), item);
        }
    }
}

fn get_addr(arg: raw::arg_t) -> usize {
    match HandlerArg::from(arg) {
        HandlerArg::Addr(addr) => addr,
        x @ _ => panic!(
            "Try to get an address from a non-address context arg {:?}",
            x
        ),
    }
}

fn get_val(arg: raw::arg_t) -> u64 {
    match HandlerArg::from(arg) {
        HandlerArg::U8(val) => val as u64,
        HandlerArg::U16(val) => val as u64,
        HandlerArg::U32(val) => val as u64,
        HandlerArg::U64(val) => val,
        HandlerArg::Addr(addr) => addr as u64,
        HandlerArg::None => panic!("No value"),
        HandlerArg::U128(_) => panic!("Does not support 128-bit integer"),
    }
}

//
// States
//
#[derive(Encode, Decode, Debug)]
struct Config {
    enabled: AtomicBool,
}

impl Marshable for Config {
    fn print(&self) {
        info!("enabled: {}", self.enabled.load(Ordering::Relaxed))
    }
}

#[derive(Encode, Decode, Debug)]
struct Persistent {
    tasks: FxHashMap<TaskId, Modify>,
}

impl Marshable for Persistent {
    fn print(&self) {
        for (id, modify) in self.tasks.iter() {
            info!("task {} {:?}", id, modify);
        }
    }
}

//
// Flags
//

pub static FLAG_HANDLER_CAS_ENABLED: FlagKey = FlagKey::new(
    c"FLAG_HANDLER_CAS_ENABLED",
    c"",
    c"handler-cas",
    c"",
    c"enable the CAS predictor",
    Value::Bool(false),
    &STR_CONVERTER_BOOL,
    Some(_handler_cas_enabled_callback),
);

unsafe extern "C" fn _handler_cas_enabled_callback(v: raw::value, _: *mut std::ffi::c_void) {
    HANDLER
        .cfg
        .enabled
        .store(Value::from(v).is_on(), Ordering::Relaxed);
}

//
// Registration
//
pub fn register() {
    lotto::engine::handler::register(&*HANDLER);
    lotto::brokers::statemgr::register(&*HANDLER);
}

pub fn register_flags() {
    let _ = FLAG_HANDLER_CAS_ENABLED.get();
}

//
// Interfaces
//

/// Get the modify info of a task.
pub fn get_modify(id: TaskId) -> Option<Modify> {
    HANDLER.pers.tasks.get(&id).cloned()
}

/// Get realtime modify info.
pub fn get_rt_modify(id: TaskId) -> Option<Modify> {
    HANDLER.rt_map.get(&id).cloned()
}
