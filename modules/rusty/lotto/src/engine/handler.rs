#![allow(clippy::ptr_arg)]

pub use crate::base::TaskId;

use crate::base::Category;
use crate::base::HandlerArg;
use crate::collections::FxHashMap;
use crate::engine::pubsub::CustomCatTable;
use as_any::AsAny;
use lazy_static::lazy_static;
use log::{trace, warn};
use lotto_sys as raw;
use raw::{context, context_t};
use std::ffi::CStr;
use std::num::NonZeroUsize;
use std::sync::{Mutex, MutexGuard, TryLockResult};

type DynHandler = dyn Handler + Sync;

/// A bare-bones Handler interface that closely corresponds to the C
/// counterpart.
pub trait Handler {
    /// Called during capture.
    fn handle(&mut self, _ctx: &raw::context_t, _event: &mut raw::event_t) {}

    /// Called during resume.
    fn posthandle(&mut self, _ctx: &raw::context_t) {}
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
    pub cat: Category,
}

impl ContextInfo {
    pub fn new(ctx: &context) -> Self {
        Self {
            event_args: EventArgs::new(ctx),
            pc: ctx.pc,
            cat: ctx.cat,
        }
    }

    pub fn empty_context() -> Self {
        Self {
            event_args: EventArgs::NoChanges,
            pc: 0,
            cat: Category::CAT_NONE,
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
    pub fn new(ctx: &context) -> Self {
        match ctx.cat {
            Category::CAT_BEFORE_AREAD => {
                let addr = get_addr_from_context(ctx, /* addr_id */ 0);
                let size = get_size_from_context(ctx, /* size_id */ 1);
                match addr {
                    Ok(addr) => {
                        EventArgs::AddressOnly(AddressCatEvent::BeforeARead, Address(addr), size)
                    }
                    _ => EventArgs::NoChanges,
                }
            }
            Category::CAT_BEFORE_READ => {
                let addr = get_addr_from_context(ctx, /* addr_id */ 0);
                let size = get_size_from_context(ctx, /* size_id */ 1);
                match addr {
                    Ok(addr) => {
                        EventArgs::AddressOnly(AddressCatEvent::BeforeRead, Address(addr), size)
                    }
                    _ => EventArgs::NoChanges,
                }
            }
            Category::CAT_BEFORE_RMW => {
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
            Category::CAT_BEFORE_AWRITE => {
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
            Category::CAT_BEFORE_WRITE => {
                let addr = get_addr_from_context(ctx, /* addr_id */ 0);
                let size = get_size_from_context(ctx, /* size_id */ 1);
                match addr {
                    Ok(addr) => {
                        EventArgs::AddressOnly(AddressCatEvent::BeforeWrite, Address(addr), size)
                    }
                    _ => EventArgs::NoChanges,
                }
            }
            Category::CAT_BEFORE_CMPXCHG => {
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
            Category::CAT_BEFORE_XCHG => {
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
                trace!("Unknown CAT for EventArgs: {:?}", ctx.cat);
                EventArgs::NoChanges
            }
        }
    }
}

pub enum AddressFromContextError {
    NullAddress,
    InvalidType(#[allow(dead_code)] String),
}

fn get_function_name_from_context(&ctx: &context_t) -> &'static str {
    // Safety: from_ptr must only be called on valid C null-terminated strings,
    // and its lifetime must exceed the requested lifetime.
    // This string is assigned from the C side to __func__ which
    // is a valid C string with static lifetime.
    let name = unsafe { CStr::from_ptr(ctx.func) };
    name.to_str().unwrap()
}

pub fn get_addr_from_context(
    &ctx: &context_t,
    addr_id: usize,
) -> Result<NonZeroUsize, AddressFromContextError> {
    let addr_to_watch = HandlerArg::from(ctx.args[addr_id]);
    if let HandlerArg::Addr(addr) = addr_to_watch {
        if let Ok(addr) = NonZeroUsize::try_from(addr) {
            Ok(addr)
        } else {
            Err(AddressFromContextError::NullAddress)
        }
    } else {
        let msg = format!("Handler received unexpected arg {:?}", addr_to_watch);
        warn!("{msg}");
        Err(AddressFromContextError::InvalidType(msg))
    }
}

pub fn get_size_from_context(&ctx: &context_t, size_id: usize) -> AddrSize {
    // it is passed like this on C: arg(size_t, SIZE)
    let size = match HandlerArg::from(ctx.args[size_id]) {
        HandlerArg::U64(lenght) => lenght,
        HandlerArg::U32(lenght) => lenght as u64,
        _ => {
            panic!(
                "Unsupported size_t parameter: {:?}. Expected to be U32 or U64",
                HandlerArg::from(ctx.args[size_id])
            );
        }
    };
    AddrSize(size)
}

pub fn get_value_from_context(&ctx: &context_t, value_id: usize, size: AddrSize) -> ValuesTypes {
    match HandlerArg::from(ctx.args[value_id]) {
        HandlerArg::None => ValuesTypes::None,
        HandlerArg::U8(val) => {
            assert_eq!(size, AddrSize(1));
            ValuesTypes::U8(val)
        }
        HandlerArg::U16(val) => {
            assert_eq!(size, AddrSize(2));
            ValuesTypes::U16(val)
        }
        HandlerArg::U32(val) => {
            assert_eq!(size, AddrSize(4));
            ValuesTypes::U32(val)
        }
        HandlerArg::U64(val) => {
            assert_eq!(size, AddrSize(8));
            ValuesTypes::U64(val)
        }
        HandlerArg::U128(val) => {
            assert_eq!(size, AddrSize(16));
            ValuesTypes::U128(val)
        }
        _ => {
            panic!(
                "Unsupported value type: {:?}. Expected to be U8, U16, U32, U64 or U128",
                HandlerArg::from(ctx.args[value_id])
            );
        }
    }
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
    &ctx: &context_t,
    addr: &Address,
    length: &AddrSize,
    rmw_value: ValuesTypes,
) -> ValuesTypes {
    let name = get_function_name_from_context(&ctx);
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
    pub custom_cat_table: CustomCatTable,
}

unsafe impl Send for InternalState {}

lazy_static! {
    pub static ref ENGINE_DATA: Mutex<InternalState> = Mutex::new(InternalState::default());
    pub static ref TASK_EVENT_TABLE: Mutex<FxHashMap<TaskId, ContextInfo>> =
        Mutex::new(FxHashMap::default());
}

impl<T: ExecuteArrivalHandler + ExecuteHandler + AsAny> ArrivalOrExecuteHandler for T {}
