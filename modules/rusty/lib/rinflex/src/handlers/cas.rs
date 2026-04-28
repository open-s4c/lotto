//! # CAS predictor
//!
//! Originally this handler records information to predict whether a
//! CAS operation can succeed or not, but now it is extended to record
//! all memory operations.

use std::collections::BTreeMap;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::LazyLock;
use std::sync::Mutex;

use crate::memory_access::{MemoryAccess, Modify, ModifyKind, Read, VAddr};
use bincode::{Decode, Encode};
use lotto::base::{CapturePoint, TaskId, Value};
use lotto::brokers::{Marshable, Stateful};
use lotto::cli::flags::{FlagKey, STR_CONVERTER_BOOL};
use lotto::engine::handler::{
    get_addr_from_context, get_size_from_context, get_value_from_context, Handler,
};
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
    fn update_task_from_ctx(&mut self, ctx: &CapturePoint) {
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
    fn handle(&mut self, ctx: &CapturePoint, _event: &mut raw::event_t) {
        self.update_task_from_ctx(ctx);
    }

    fn posthandle(&mut self, ctx: &CapturePoint) {
        // We need to save the "real" read value, which is only
        // available after the event is resumed.
        self.update_task_from_ctx(ctx);
    }
}

#[inline]
fn ctx_to_memory_access(ctx: &CapturePoint) -> Option<MemoryAccess> {
    ctx_to_modify(ctx)
        .map(MemoryAccess::Modify)
        .or_else(|| ctx_to_read(ctx).map(MemoryAccess::Read))
}

#[inline]
fn ctx_to_modify(ctx: &CapturePoint) -> Option<Modify> {
    let event_type = ctx.type_id as u32;
    let is_after = u32::from(ctx.chain_id) == raw::CHAIN_INGRESS_AFTER;

    if !matches!(
        event_type,
        raw::EVENT_MA_WRITE
            | raw::EVENT_MA_AWRITE
            | raw::EVENT_MA_RMW
            | raw::EVENT_MA_CMPXCHG
            | raw::EVENT_MA_CMPXCHG_WEAK
            | raw::EVENT_MA_XCHG
    ) {
        return None;
    }

    let raw_addr = match get_addr_from_context(ctx, 0) {
        Ok(addr) => usize::from(addr),
        Err(_) => return None,
    };
    let addr = VAddr::get(ctx, raw_addr);
    let size = u64::from(get_size_from_context(ctx, 1));
    let kind: ModifyKind = match event_type {
        raw::EVENT_MA_CMPXCHG | raw::EVENT_MA_CMPXCHG_WEAK => ModifyKind::Cas {
            is_after,
            expected: get_ctx_val(ctx, 2, size),
            new: get_ctx_val(ctx, 3, size),
        },

        raw::EVENT_MA_RMW => ModifyKind::Rmw {
            is_after,
            delta: get_ctx_val(ctx, 2, size),
            operator: {
                let val = ctx.rmw_op().expect("missing RMW operator from context");
                // Safety: it's coming from the interceptor directly
                unsafe { std::mem::transmute(val) }
            },
        },

        raw::EVENT_MA_XCHG => ModifyKind::Xchg {
            is_after,
            value: get_ctx_val(ctx, 2, size),
        },

        raw::EVENT_MA_WRITE => ModifyKind::Write,

        raw::EVENT_MA_AWRITE => ModifyKind::AWrite {
            is_after,
            value: get_ctx_val(ctx, 2, size),
        },

        // Ignore other non-modifying operations
        _ => panic!("Some modify categories are not handled"),
    };

    // NOTE: We need to get the value early for the order enforcer to
    // detect whether the event should be blocked.
    let read_value = if matches!(
        event_type,
        raw::EVENT_MA_RMW | raw::EVENT_MA_CMPXCHG | raw::EVENT_MA_CMPXCHG_WEAK | raw::EVENT_MA_XCHG
    ) {
        // 1. Currently, only consider CAS, XCHG and RMW events.
        // 2. An AFTER event is understood as the same operation as the BEFORE event, so they use the same value.
        if !is_after {
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
fn ctx_to_read(_ctx: &raw::capture_point) -> Option<Read> {
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

fn get_ctx_val(ctx: &CapturePoint, value_id: usize, size: u64) -> u64 {
    match get_value_from_context(ctx, value_id, lotto::engine::handler::AddrSize::new(size)) {
        lotto::engine::handler::ValuesTypes::U8(val) => val as u64,
        lotto::engine::handler::ValuesTypes::U16(val) => val as u64,
        lotto::engine::handler::ValuesTypes::U32(val) => val as u64,
        lotto::engine::handler::ValuesTypes::U64(val) => val,
        lotto::engine::handler::ValuesTypes::U128(_) => {
            panic!("Does not support 128-bit integer")
        }
        lotto::engine::handler::ValuesTypes::None => panic!("No value"),
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
    c"rusty-cas",
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
