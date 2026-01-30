//! # CAS predictor
//!
//! Originally this handler records information to predict whether a
//! CAS operation can succeed or not, but now it is extended to record
//! all memory operations.

use std::collections::BTreeMap;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::LazyLock;
use std::sync::Mutex;

use crate::memory_access::{MemoryAccess, MemoryOperationExt, Modify, ModifyKind, Read, VAddr};
use bincode::{Decode, Encode};
use lotto::base::category::Category;
use lotto::base::{HandlerArg, Value};
use lotto::brokers::{Marshable, Stateful};
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
        tasks: BTreeMap::new(),
    },
    rt_map: Mutex::new(BTreeMap::new()),
});

#[derive(Stateful)]
struct CasPredictor {
    #[config]
    cfg: Config,
    #[persistent]
    pers: Persistent,

    // Real-time values from the current execution.
    rt_map: Mutex<BTreeMap<TaskId, MemoryAccess>>,
}

impl CasPredictor {
    #[inline]
    fn update_task_from_ctx(&mut self, ctx: &raw::context_t) {
        if !self.cfg.enabled.load(Ordering::Relaxed) {
            return;
        }
        if let Some(ma) = ctx_to_memory_access(ctx) {
            self.pers.tasks.insert(TaskId(ctx.id), ma.clone());
            self.rt_map
                .try_lock()
                .expect("single-thread")
                .insert(TaskId(ctx.id), ma.clone());
        } else {
            // Not a memory access. They must not be present in the
            // trace, or other non-memory-access events will be
            // associated a value!
            self.pers.tasks.remove(&TaskId(ctx.id));
            self.rt_map
                .try_lock()
                .expect("single-thread")
                .remove(&TaskId(ctx.id));
        }
    }
}

//
// Handler
//
impl Handler for CasPredictor {
    fn handle(&mut self, ctx: &raw::context_t, _event: &mut raw::event_t) {
        self.update_task_from_ctx(ctx);
    }

    fn posthandle(&mut self, ctx: &raw::context_t) {
        // We need to save the "real" read value, which is only
        // available after the event is resumed.
        self.update_task_from_ctx(ctx);
    }
}

#[inline]
fn ctx_to_memory_access(ctx: &raw::context_t) -> Option<MemoryAccess> {
    ctx_to_modify(ctx)
        .map(MemoryAccess::Modify)
        .or_else(|| ctx_to_read(ctx).map(MemoryAccess::Read))
}

#[inline]
fn ctx_to_modify(ctx: &raw::context_t) -> Option<Modify> {
    if !ctx.cat.is_write() {
        return None;
    }

    let raw_addr = get_addr(ctx.args[0]);
    let addr = VAddr::get(ctx, raw_addr);
    let size = get_val(ctx.args[1]);
    let is_after = ctx.cat.is_after();

    let kind: ModifyKind = match ctx.cat {
        // CAS
        Category::CAT_BEFORE_CMPXCHG
        | Category::CAT_AFTER_CMPXCHG_S
        | Category::CAT_AFTER_CMPXCHG_F => ModifyKind::Cas {
            is_after,
            expected: get_val(ctx.args[2]),
            new: get_val(ctx.args[3]),
        },

        // RMW
        Category::CAT_BEFORE_RMW | Category::CAT_AFTER_RMW => ModifyKind::Rmw {
            is_after,
            delta: get_val(ctx.args[2]),
            operator: {
                let val = get_val(ctx.args[3]) as u32;
                // Safety: it's coming from the interceptor directly
                unsafe { std::mem::transmute(val) }
            },
        },

        // XCHG
        Category::CAT_BEFORE_XCHG | Category::CAT_AFTER_XCHG => ModifyKind::Xchg {
            is_after,
            value: get_val(ctx.args[2]),
        },

        // Plain writes
        Category::CAT_BEFORE_WRITE => ModifyKind::Write,

        // Atomic writes
        Category::CAT_BEFORE_AWRITE | Category::CAT_AFTER_AWRITE => ModifyKind::AWrite {
            is_after,
            value: get_val(ctx.args[2]),
        },

        // Ignore other non-modifying operations
        _ => panic!("Some modify categories are not handled"),
    };

    // NOTE: We need to get the value early for the order enforcer to
    // detect whether the event should be blocked.
    let read_value = if ctx.cat.is_read() {
        // 1. Currently, only consider XCHG and RMW events.
        // 2. An AFTER event is understood as the same operation as the BEFORE event, so they use the same value.
        if ctx.cat.is_before() && (ctx.cat.is_xchg() || ctx.cat.is_rmw()) {
            let value = unsafe { crate::sized_read(raw_addr as u64, size as usize) };
            Some(value)
        } else {
            get_memory_access_value(TaskId(ctx.id))
        }
    } else {
        None
    };

    let item = Modify {
        addr,
        real_addr: raw_addr as u64,
        size: size as u8,
        kind,
        read_value,
    };

    Some(item)
}

/// `Read` events are not recorded.
#[inline]
fn ctx_to_read(_ctx: &raw::context_t) -> Option<Read> {
    return None;
    // let (addr, size) = match ctx.cat {
    //     Category::CAT_BEFORE_READ | Category::CAT_BEFORE_AREAD | Category::CAT_AFTER_AREAD => {
    //         (get_val(ctx.args[0]), get_val(ctx.args[1]))
    //     }
    //     _ => {
    //         return None;
    //     }
    // };
    // let value = unsafe { crate::sized_read(addr, size as usize) };
    // Some(Read {
    //     size: size as u8,
    //     real_addr: addr,
    //     addr: VAddr::get(ctx, addr as usize),
    //     value,
    // })
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
    tasks: BTreeMap<TaskId, MemoryAccess>,
}

impl Marshable for Persistent {
    fn print(&self) {
        for (id, ma) in self.tasks.iter() {
            info!("task {} {:?}", id, ma);
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

/// Get the memory access info of a task.
pub fn get_memory_access(id: TaskId) -> Option<MemoryAccess> {
    HANDLER.pers.tasks.get(&id).cloned()
}

/// Get the real-time memory access info of a task.
pub fn get_rt_memory_access(id: TaskId) -> Option<MemoryAccess> {
    HANDLER
        .rt_map
        .try_lock()
        .expect("single thread")
        .get(&id)
        .cloned()
}

/// Get the memory access's value of a task.
pub fn get_memory_access_value(id: TaskId) -> Option<u64> {
    get_memory_access(id).map(|ma| ma.loaded_value()).flatten()
}

/// Update memory values
pub fn update_memory_access(id: TaskId) {
    let pers = unsafe { &mut *HANDLER.persistent_ptr() };
    if let Some(ma) = pers.tasks.get_mut(&id) {
        update(ma, id);
    }
    let mut rt_map = HANDLER.rt_map.try_lock().expect("single-thread");
    if let Some(ma) = rt_map.get_mut(&id) {
        update(ma, id);
    }
}

fn update(ma: &mut MemoryAccess, id: TaskId) {
    match ma {
        MemoryAccess::Read(r) => {
            r.value = unsafe { r.reload_value() };
        }
        MemoryAccess::Modify(
            m @ Modify {
                read_value: Some(_),
                ..
            },
        ) => {
            if !m.kind.is_after() {
                m.read_value = unsafe { m.reload_value() };
            } else {
                // BEFORE's value should have been updated.
                m.read_value = get_memory_access_value(id);
            }
        }
        _ => {}
    }
}
