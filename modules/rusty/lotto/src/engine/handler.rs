#![allow(clippy::ptr_arg)]

use crate::base::{CapturePoint, TaskId};
use crate::collections::FxHashMap;
use crate::engine::pubsub::CustomEventTable;
use as_any::AsAny;
use lazy_static::lazy_static;
use log::{trace, warn};
use lotto_sys as raw;
use std::num::NonZeroUsize;
use std::sync::{Mutex, MutexGuard, TryLockResult};

type DynHandler = dyn Handler + Sync;

/// A bare-bones Handler interface that closely corresponds to the C
/// counterpart.
pub trait Handler {
    /// Called during capture.
    fn handle(&mut self, _ctx: &CapturePoint, _event: &mut raw::event_t) {}

    /// Called during resume.
    fn posthandle(&mut self, _ctx: &CapturePoint) {}
}

pub(crate) struct HandlerList {
    handlers: Mutex<Vec<*mut DynHandler>>,
}

// Safety: `HandlerList` guards itself from concurrent access.
unsafe impl Sync for HandlerList {}

impl HandlerList {
    pub fn new() -> HandlerList {
        HandlerList {
            handlers: Mutex::new(vec![]),
        }
    }

    pub fn try_lock(&self) -> TryLockResult<MutexGuard<'_, Vec<*mut DynHandler>>> {
        self.handlers.try_lock()
    }
}

lazy_static! {
    pub(crate) static ref HANDLER_LIST: HandlerList = HandlerList::new();
}

/// Register [`Handler`].
///
/// They will be called in order.
pub fn register(handler: &'static DynHandler) {
    let mut handlers = HANDLER_LIST.handlers.try_lock().expect("single threaded");
    handlers.push(handler as *const _ as *mut _);
}

/// Trait for all the handlers that must know the exact order of thread
/// execution, and allow it to even get the value at the address that it
/// will read or write on, for example.
pub trait ExecuteHandler {
    /// Processes the event right before it gets executed.
    ///
    /// The scheduling decision has already been made and will not
    /// change between the time this handler gets called, and the
    /// actual event execution.
    fn handle_execute(&mut self, tid: TaskId, ctx_info: &ContextInfo);

    /// Returns if the handler is enabled or not.
    ///
    /// For toggle options you must override this function and is
    /// recommended to use config states.
    fn enabled(&self) -> bool {
        true
    }
}

pub trait ExecuteArrivalHandler {
    /// processes the event as soon as the thread reaches the capture point
    /// of the event. There is no guarantee that this event will be executed
    /// any time soon as the scheduler may change the running thread
    /// between the time this handler is called, and the time the next event
    /// is actually executed. This handler can influence the scheduler.
    fn handle_arrival(
        &mut self,
        tid: TaskId,
        ctx_info: &ContextInfo,
        active_tids: &Vec<TaskId>,
    ) -> EventResult;
}

/// Handles both arrival at capture points, and execution of events.
pub trait ArrivalOrExecuteHandler: ExecuteHandler + ExecuteArrivalHandler + AsAny {}

#[derive(Debug)]
pub struct ContextInfo {
    pub event_args: EventArgs,
    pub pc: usize,
    pub type_id: u32,
}

impl ContextInfo {
    pub fn new(ctx: &CapturePoint) -> Self {
        Self {
            event_args: EventArgs::new(ctx),
            pc: ctx.pc,
            type_id: ctx.event_type().to_raw_value(),
        }
    }

    pub fn empty_context() -> Self {
        Self {
            event_args: EventArgs::NoChanges,
            pc: 0,
            type_id: 0,
        }
    }

    pub fn get_cas_addr_and_expected_value(&self) -> Option<(Address, AddrSize, ValuesTypes)> {
        match self.event_args {
            EventArgs::AddressTwoValuesEvent(
                AddressTwoValuesCatEvent::BeforeCMPXCHG,
                addr,
                lenght,
                expected,
                _,
            ) => Some((addr, lenght, expected)),
            _ => None,
        }
    }
}

/// Contains the result of a handler execution that will be used to
/// inform the scheduler of necessary changes.
///
/// Note that if the event is read-only this is not propagated.  Also
/// note that we dont overwrite a field by a "weaker" result, for
/// example, if cappt->is_chpt is true, it wont change to false
/// regardless of the value of `is_change_point`.
pub struct EventResult {
    /// List of tasks_id to be blocked in the following
    /// This is only a hint, because if the capture point is
    /// read-only, we can't change it.
    blocked_tasks: Vec<TaskId>,
    /// Indicates that this event is a change point, if the
    /// capture point is NOT read-only.
    is_change_point: bool,
    /// Indicates the reason of the action, for example, of shutdown.
    /// `None` if not set.
    /// Note that if other handler already changed the reason or if the
    /// event is read_only this will have no effect.
    reason: Option<Reason>,
    /// Indicates that we want to change the capture point to read only.
    /// It may be important to check the order of the rust handlers if
    /// you want to use this.
    read_only: bool,
    next_task_to_run: Option<TaskId>,
}

impl EventResult {
    pub fn new(blocked_tasks: Vec<TaskId>, is_change_point: bool) -> Self {
        Self {
            blocked_tasks,
            is_change_point,
            reason: None,
            read_only: false,
            next_task_to_run: None,
        }
    }

    /// Creates a event result that marks `next_thread_to_run`, and gives the reason.
    ///
    /// In addition it supports schedules of no threads or abort/shutdown schedules by
    /// passing the reason only.
    pub fn new_from_schedule(next_tid: Option<TaskId>, reason: Option<Reason>) -> Self {
        let mut event_result = EventResult::new(/*blocked_tasks*/ Vec::new(), false);
        event_result.set_read_only();
        if let Some(reason) = reason {
            match reason {
                Reason::Deterministic
                | Reason::Abort(AbortReason::ReasonAbort)
                | Reason::Shutdown(_) => { /* supported */ }
                _ => {
                    panic!(
                        "Reason not support: {:?}. Please create the EventResult manually.",
                        reason
                    )
                }
            }
            event_result.set_reason(reason);
        }
        if let Some(next_tid) = next_tid {
            event_result.set_next_task_to_run(next_tid);
        }
        event_result
    }

    pub fn set_reason(&mut self, reason: Reason) {
        self.reason = Some(reason);
    }
    /// Marks the event as read only. Note that in most case we DONT need this.
    /// By setting the reason we make sure that it will not be overwritten further.
    pub fn set_read_only(&mut self) {
        self.read_only = true;
    }
    /// Marks that we want to force `tid` to run next. This may not apply.
    pub fn set_next_task_to_run(&mut self, tid: TaskId) {
        self.next_task_to_run = Some(tid);
    }
    pub fn to_tuple(&self) -> (&Vec<TaskId>, &bool, &Option<Reason>, &bool, &Option<TaskId>) {
        (
            &self.blocked_tasks,
            &self.is_change_point,
            &self.reason,
            &self.read_only,
            &self.next_task_to_run,
        )
    }
}

#[derive(Debug, PartialEq, Clone, Copy, Serialize, Deserialize)]
pub enum ValuesTypes {
    None,
    U8(u8),
    U16(u16),
    U32(u32),
    U64(u64),
    U128(u128),
}

impl From<ValuesTypes> for u64 {
    fn from(val: ValuesTypes) -> Self {
        match val {
            ValuesTypes::U8(v) => v as u64,
            ValuesTypes::U16(v) => v as u64,
            ValuesTypes::U32(v) => v as u64,
            ValuesTypes::U64(v) => v,
            _ => panic!("unexpected ValueType in RMW event: {:?}", val),
        }
    }
}

use serde_derive::{Deserialize, Serialize};

#[derive(Debug, Eq, PartialEq, Copy, Clone, Hash, PartialOrd, Ord, Serialize, Deserialize)]
pub struct Address(NonZeroUsize);

impl Address {
    pub fn new(addr: NonZeroUsize) -> Self {
        Self(addr)
    }
}

impl From<Address> for NonZeroUsize {
    fn from(addr: Address) -> Self {
        addr.0
    }
}

#[derive(Debug, Copy, Clone, PartialEq, Serialize, Deserialize)]
pub struct AddrSize(u64);

impl From<AddrSize> for u64 {
    fn from(size: AddrSize) -> Self {
        size.0
    }
}

impl AddrSize {
    pub fn new(sz: u64) -> Self {
        Self(sz)
    }
}

#[derive(Debug, PartialEq, Eq)]

pub enum AddressCatEvent {
    BeforeARead,
    BeforeRead,
    BeforeWrite,
}
#[derive(Debug, PartialEq, Eq)]

pub enum AddressValueCatEvent {
    BeforeAWrite,
    BeforeRMW,
}
#[derive(Debug)]
pub enum AddressTwoValuesCatEvent {
    BeforeCMPXCHG,
}

///
/// Keeps for each important event all the needed context information
/// If something changes in the interceptor_tsan, we must also update it here
/// For now, we only care about BEFORE and CUSTOM events.
///
/// Note: the `ValueTypes` passed refeers to the value that WILL be written.
/// For CAS this is also the value passed in the context. For other
/// RMW operations it isn't always the case.
///
#[derive(Debug)]
pub enum EventArgs {
    /// The arguments are, in this order:
    /// Category, Address, Leght of pointer, new written value.
    AddressValue(AddressValueCatEvent, Address, AddrSize, ValuesTypes),
    AddressOnly(AddressCatEvent, Address, AddrSize),
    AddressTwoValuesEvent(
        AddressTwoValuesCatEvent,
        Address,
        AddrSize,
        ValuesTypes,
        ValuesTypes,
    ),
    NoChanges,
}

impl EventArgs {
    pub fn new(ctx: &CapturePoint) -> Self {
        match ctx.event_type().to_raw_value() {
            raw::EVENT_MA_AREAD
                if u32::from(ctx.chain_id) != raw::CHAIN_INGRESS_AFTER =>
            {
                let addr = get_addr_from_context(ctx, /* addr_id */ 0);
                let size = get_size_from_context(ctx, /* size_id */ 1);
                match addr {
                    Ok(addr) => {
                        EventArgs::AddressOnly(AddressCatEvent::BeforeARead, Address(addr), size)
                    }
                    _ => EventArgs::NoChanges,
                }
            }
            raw::EVENT_MA_READ => {
                let addr = get_addr_from_context(ctx, /* addr_id */ 0);
                let size = get_size_from_context(ctx, /* size_id */ 1);
                match addr {
                    Ok(addr) => {
                        EventArgs::AddressOnly(AddressCatEvent::BeforeRead, Address(addr), size)
                    }
                    _ => EventArgs::NoChanges,
                }
            }
            raw::EVENT_MA_RMW if u32::from(ctx.chain_id) != raw::CHAIN_INGRESS_AFTER => {
                let addr = match get_addr_from_context(ctx, /* addr_id */ 0) {
                    Ok(addr) => Address(addr),
                    _ => return EventArgs::NoChanges,
                };
                let size = get_size_from_context(ctx, /* size_id */ 1);
                let value_in_ctx = get_value_from_context(ctx, /* value_id */ 2, size);
                // For RMW this value is not necessaraly the new written value. We must simulate
                // the desired function to get the actual new value.
                let new_value = get_new_value_after_rmw(ctx, &addr, &size, value_in_ctx);
                EventArgs::AddressValue(AddressValueCatEvent::BeforeRMW, addr, size, new_value)
            }
            raw::EVENT_MA_AWRITE
                if u32::from(ctx.chain_id) != raw::CHAIN_INGRESS_AFTER =>
            {
                let addr = get_addr_from_context(ctx, /* addr_id */ 0);
                let size = get_size_from_context(ctx, /* size_id */ 1);
                let value =
                    get_value_from_context(ctx, /* value_id */ 2, /* size */ size);
                match addr {
                    Ok(addr) => EventArgs::AddressValue(
                        AddressValueCatEvent::BeforeAWrite,
                        Address(addr),
                        size,
                        value,
                    ),
                    _ => EventArgs::NoChanges,
                }
            }
            raw::EVENT_MA_WRITE => {
                let addr = get_addr_from_context(ctx, /* addr_id */ 0);
                let size = get_size_from_context(ctx, /* size_id */ 1);
                match addr {
                    Ok(addr) => {
                        EventArgs::AddressOnly(AddressCatEvent::BeforeWrite, Address(addr), size)
                    }
                    _ => EventArgs::NoChanges,
                }
            }
            raw::EVENT_MA_CMPXCHG | raw::EVENT_MA_CMPXCHG_WEAK
                if u32::from(ctx.chain_id) != raw::CHAIN_INGRESS_AFTER =>
            {
                let addr = get_addr_from_context(ctx, /* addr_id */ 0);
                let size = get_size_from_context(ctx, /* size_id */ 1);
                let value1 =
                    get_value_from_context(ctx, /* value_id */ 2, /* size */ size);
                let value2 =
                    get_value_from_context(ctx, /* value_id */ 3, /* size */ size);

                match addr {
                    Ok(addr) => EventArgs::AddressTwoValuesEvent(
                        AddressTwoValuesCatEvent::BeforeCMPXCHG,
                        Address(addr),
                        size,
                        value1,
                        value2,
                    ),
                    _ => EventArgs::NoChanges,
                }
            }
            raw::EVENT_MA_XCHG if u32::from(ctx.chain_id) != raw::CHAIN_INGRESS_AFTER => {
                let addr = get_addr_from_context(ctx, /* addr_id */ 0);
                let size = get_size_from_context(ctx, /* size_id */ 1);
                let value = get_value_from_context(ctx, /* value_id */ 2, size);
                match addr {
                    Ok(addr) => EventArgs::AddressValue(
                        AddressValueCatEvent::BeforeRMW,
                        Address(addr),
                        size,
                        value,
                    ),
                    _ => EventArgs::NoChanges,
                }
            }
            _ => {
                trace!("Unknown event type for EventArgs: {:?}", ctx.event_type());
                EventArgs::NoChanges
            }
        }
    }
}

#[derive(Debug)]
pub enum AddressFromContextError {
    NullAddress,
    InvalidType(#[allow(dead_code)] String),
}

fn value_from_size(size: AddrSize, value: u64) -> ValuesTypes {
    match u64::from(size) {
        1 => ValuesTypes::U8(value as u8),
        2 => ValuesTypes::U16(value as u16),
        4 => ValuesTypes::U32(value as u32),
        8 => ValuesTypes::U64(value),
        _ => panic!("Unsupported value width {}", u64::from(size)),
    }
}

pub fn get_addr_from_context(
    ctx: &CapturePoint,
    addr_id: usize,
) -> Result<NonZeroUsize, AddressFromContextError> {
    let addr = match ctx.event_type().to_raw_value() {
        raw::EVENT_AWAIT => unsafe {
            ctx.payload_unchecked::<raw::await_event_t>()
                .map(|ev| ev.addr as usize)
        },
        raw::EVENT_STACKTRACE_ENTER => ctx.caller_pc(),
        raw::EVENT_MA_READ => unsafe {
            ctx.payload_unchecked::<raw::ma_read_event>()
                .map(|ev| ev.addr as usize)
        },
        raw::EVENT_MA_WRITE => unsafe {
            ctx.payload_unchecked::<raw::ma_write_event>()
                .map(|ev| ev.addr as usize)
        },
        raw::EVENT_MA_AREAD => unsafe {
            ctx.payload_unchecked::<raw::ma_aread_event>()
                .map(|ev| ev.addr as usize)
        },
        raw::EVENT_MA_AWRITE => unsafe {
            ctx.payload_unchecked::<raw::ma_awrite_event>()
                .map(|ev| ev.addr as usize)
        },
        raw::EVENT_MA_RMW => unsafe {
            ctx.payload_unchecked::<raw::ma_rmw_event>()
                .map(|ev| ev.addr as usize)
        },
        raw::EVENT_MA_XCHG => unsafe {
            ctx.payload_unchecked::<raw::ma_xchg_event>()
                .map(|ev| ev.addr as usize)
        },
        raw::EVENT_MA_CMPXCHG | raw::EVENT_MA_CMPXCHG_WEAK => unsafe {
            ctx.payload_unchecked::<raw::ma_cmpxchg_event>()
                .map(|ev| ev.addr as usize)
        },
        _ => None,
    };

    if let Some(addr) = addr {
        return NonZeroUsize::try_from(addr).map_err(|_| AddressFromContextError::NullAddress);
    }

    let msg = format!(
        "Handler received no address payload for event type {}",
        ctx.event_type()
    );
    warn!("{msg}");
    let _ = addr_id;
    Err(AddressFromContextError::InvalidType(msg))
}

pub fn get_size_from_context(ctx: &CapturePoint, size_id: usize) -> AddrSize {
    let size = match ctx.event_type().to_raw_value() {
        raw::EVENT_MA_READ => unsafe {
            ctx.payload_unchecked::<raw::ma_read_event>()
                .map(|ev| ev.size as u64)
        },
        raw::EVENT_MA_WRITE => unsafe {
            ctx.payload_unchecked::<raw::ma_write_event>()
                .map(|ev| ev.size as u64)
        },
        raw::EVENT_MA_AREAD => unsafe {
            ctx.payload_unchecked::<raw::ma_aread_event>()
                .map(|ev| ev.size as u64)
        },
        raw::EVENT_MA_AWRITE => unsafe {
            ctx.payload_unchecked::<raw::ma_awrite_event>()
                .map(|ev| ev.size as u64)
        },
        raw::EVENT_MA_RMW => unsafe {
            ctx.payload_unchecked::<raw::ma_rmw_event>()
                .map(|ev| ev.size as u64)
        },
        raw::EVENT_MA_XCHG => unsafe {
            ctx.payload_unchecked::<raw::ma_xchg_event>()
                .map(|ev| ev.size as u64)
        },
        raw::EVENT_MA_CMPXCHG | raw::EVENT_MA_CMPXCHG_WEAK => unsafe {
            ctx.payload_unchecked::<raw::ma_cmpxchg_event>()
                .map(|ev| ev.size as u64)
        },
        _ => None,
    };
    if let Some(size) = size {
        return AddrSize(size);
    }

    let _ = size_id;
    panic!(
        "Unsupported size payload for event type {}",
        ctx.event_type()
    );
}

pub fn get_value_from_context(ctx: &CapturePoint, value_id: usize, size: AddrSize) -> ValuesTypes {
    let event_type = ctx.event_type();
    let value = match event_type.to_raw_value() {
        raw::EVENT_SPIN_END => unsafe {
            ctx.payload_unchecked::<raw::spin_end_event_t>()
                .map(|ev| ValuesTypes::U32(ev.cond))
        },
        raw::EVENT_MA_AWRITE => unsafe {
            ctx.payload_unchecked::<raw::ma_awrite_event>()
                .map(|ev| value_from_size(size, ev.val.u64_))
        },
        raw::EVENT_MA_RMW => unsafe {
            ctx.payload_unchecked::<raw::ma_rmw_event>().map(|ev| {
                if value_id == 2 {
                    value_from_size(size, ev.val.u64_)
                } else {
                    ValuesTypes::U32(ev.op as u32)
                }
            })
        },
        raw::EVENT_MA_XCHG => unsafe {
            ctx.payload_unchecked::<raw::ma_xchg_event>()
                .map(|ev| value_from_size(size, ev.val.u64_))
        },
        raw::EVENT_MA_CMPXCHG | raw::EVENT_MA_CMPXCHG_WEAK => unsafe {
            ctx.payload_unchecked::<raw::ma_cmpxchg_event>().map(|ev| {
                if value_id == 2 {
                    value_from_size(size, ev.cmp.u64_)
                } else {
                    value_from_size(size, ev.val.u64_)
                }
            })
        },
        _ => None,
    };
    if let Some(value) = value {
        return value;
    }

    panic!(
        "Unsupported value payload for event type {} (value_id={value_id}, size={})",
        ctx.event_type(),
        u64::from(size)
    );
}

pub fn get_value_from_addr(addr: &Address, length: &AddrSize) -> ValuesTypes {
    let addr = NonZeroUsize::from(*addr);
    let length = u64::from(*length);
    match length {
        1 => {
            let value = unsafe { *(usize::from(addr) as *const u8) };
            ValuesTypes::U8(value)
        }
        2 => {
            let value = unsafe { *(usize::from(addr) as *const u16) };
            ValuesTypes::U16(value)
        }
        4 => {
            let value = unsafe { *(usize::from(addr) as *const u32) };
            ValuesTypes::U32(value)
        }
        8 => {
            let value = unsafe { *(usize::from(addr) as *const u64) };
            ValuesTypes::U64(value)
        }
        16 => {
            let value = unsafe { *(usize::from(addr) as *const u128) };
            ValuesTypes::U128(value)
        }
        _ => {
            panic!(
                "Invalid length of pointer {length}. Only supported for u8, u16, u32, u64 and u128"
            );
        }
    }
}

fn get_new_value_after_rmw(
    ctx: &CapturePoint,
    addr: &Address,
    length: &AddrSize,
    rmw_value: ValuesTypes,
) -> ValuesTypes {
    let name = ctx.function_name();
    let cur_value = get_value_from_addr(addr, length);

    if cur_value == ValuesTypes::None {
        return ValuesTypes::None;
    }
    let cur_value: u64 = u64::from(cur_value);
    let rmw_value: u64 = u64::from(rmw_value);
    // RMW operations are only defined on unsigned integers. The
    // behavior of overflowing is defined.
    let new_value = if name.contains("_add") {
        cur_value.overflowing_add(rmw_value).0
    } else if name.contains("_sub") {
        cur_value.overflowing_sub(rmw_value).0
    } else if name.contains("_nand") {
        !(cur_value & rmw_value)
    } else if name.contains("_and") {
        cur_value & rmw_value
    } else if name.contains("_xor") {
        cur_value ^ rmw_value
    } else if name.contains("_or") {
        cur_value | rmw_value
    } else {
        panic!("Unsupported RMW operation {:?}", name);
    };
    match length {
        AddrSize(1) => ValuesTypes::U8(new_value as u8),
        AddrSize(2) => ValuesTypes::U16(new_value as u16),
        AddrSize(4) => ValuesTypes::U32(new_value as u32),
        AddrSize(8) => ValuesTypes::U64(new_value as u64),
        _ => panic!("invalid size"),
    }
}

/// Rust counterpart to cappt->reason.
/// See `lotto/base/reason.h` for more details.
#[derive(Clone, Copy, PartialEq, Debug)]
pub enum Reason {
    Shutdown(ShutdownReason),
    Abort(AbortReason),
    Deterministic,
}

#[derive(Clone, Copy, PartialEq, Debug)]
pub enum ShutdownReason {
    ReasonShutdown,
    ReasonSuccess,
    ReasonIgnore,
}

#[derive(Clone, Copy, PartialEq, Debug)]
pub enum AbortReason {
    ReasonAssertFail,
    ReasonResourceDeadlock,
    ReasonSegfault,
    ReasonSigint,
    ReasonSigabrt,
    ReasonAbort,
    ReasonRutimeSegfault,
    ReasonRuntimeSigint,
    ReasonRuntimeSigabrt,
}

impl Reason {
    pub fn is_shutdown_or_abort(reason: &Reason) -> bool {
        matches!(reason, Reason::Abort(_) | Reason::Shutdown(_))
    }
}

pub type DynExecuteHandler = dyn ExecuteHandler + Send + Sync;
pub type DynArrivalOrExecuteHandler = dyn ArrivalOrExecuteHandler + Send + Sync;

#[derive(Default)]
pub struct InternalState {
    pub execute_handlers_list: Vec<*mut DynExecuteHandler>,
    pub arrival_or_execute_handlers_list: Vec<*mut DynArrivalOrExecuteHandler>,
    pub custom_event_table: CustomEventTable,
}

unsafe impl Send for InternalState {}

lazy_static! {
    pub static ref ENGINE_DATA: Mutex<InternalState> = Mutex::new(InternalState::default());
    pub static ref TASK_EVENT_TABLE: Mutex<FxHashMap<TaskId, ContextInfo>> =
        Mutex::new(FxHashMap::default());
}

impl<T: ExecuteArrivalHandler + ExecuteHandler + AsAny> ArrivalOrExecuteHandler for T {}
