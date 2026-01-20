//! A Lotto Handler that will build a bridge between events received from
//! from the C side, and publish the events to the subscribed handlers.
//!
//!
use crate::base::TaskId;
use crate::engine::handler::AbortReason::*;
use crate::engine::handler::ShutdownReason::*;
use crate::engine::handler::{ContextInfo, EventResult, Reason};
use log::warn;
use log::{debug, trace};

use crate::collections::FxHashMap;
use crate::engine::handler::{
    ArrivalOrExecuteHandler, DynArrivalOrExecuteHandler, DynExecuteHandler,
};
use crate::raw::reason::REASON_DETERMINISTIC;
use crate::raw::reason::*;
use crate::raw::selector_SELECTOR_FIRST;
use crate::raw::{
    base_category, context_t, dispatcher_register, event_t, task_id, tidset_remove, tidset_t,
};
use lotto_sys as raw;
use std::mem::align_of;
use std::sync::atomic::{AtomicBool, Ordering};

pub type CustomCatTable =
    FxHashMap<u32, Box<dyn Fn(&context_t) -> Box<dyn CustomCatTrait> + Send + Sync>>;

pub trait CustomCatTrait {
    fn call_right_handler(&self, handler: &mut (dyn ArrivalOrExecuteHandler + Send + Sync));
}

/// Publish the arrival of an event to all the subscribed handlers.
///
/// For example, in the event of a CAT_BEFORE_AWRITE it will
/// publish this event as soon as the category is reached, which is
/// before the write itself.
///
/// # Safety
///
/// This function is unsafe because it retrieves the context and
/// the capture_point event that are received from the C side.
/// The user must ensure the pointers are alligned, not-NULL and
/// properly initialized.
///
pub unsafe extern "C" fn publish_arrival(ctx: *const context_t, event: *mut event_t) {
    assert_eq!(
        (ctx as usize) % align_of::<context_t>(),
        0,
        "Invalid alignment"
    );
    assert_eq!(
        (event as usize) % align_of::<event_t>(),
        0,
        "Invalid alignment"
    );

    let _guard = DispatchGuard::enter();

    // SAFETY: We checked the alignment, that the pointer is not-null and trust the caller to
    // provide us with a properly initialized context.
    let ctx: &context_t = unsafe { ctx.as_ref() }.expect("Unexpected null pointer");
    let tid = TaskId::new(ctx.id);
    let cat = ctx.cat;
    let ctx_info: ContextInfo = ContextInfo::new(ctx);

    // Call simple handlers
    {
        let mut handlers = crate::engine::handler::HANDLER_LIST
            .try_lock()
            .expect("single threaded");
        for handler_ptr in handlers.iter_mut() {
            let handler = unsafe { &mut **handler_ptr };
            let event_ref = unsafe { &mut *event };
            handler.handle(ctx, event_ref);
        }
    }

    // Call complex handlers
    trace!(
        "Saving to table: task {:?} with event {:?}. Category = {:?}",
        tid,
        ctx_info,
        cat
    );

    let table = &mut crate::engine::handler::TASK_EVENT_TABLE
        .try_lock()
        .expect("Single threaded appliaction");

    let util_data = &mut crate::engine::handler::ENGINE_DATA
        .try_lock()
        .expect("single threaded");

    assert!(!event.is_null());
    // SAFETY: We checked the alignment, that the pointer is not-null and trust the caller to
    // provide us with a properly initialized event.
    let received_read_only = unsafe { (*event).readonly };
    let mut cur_read_only = received_read_only;

    let mut reason: Option<Reason> = None;
    let mut blocked_tasks: Vec<TaskId> = Vec::new();
    let mut next_task_to_run: Option<TaskId> = None;
    let mut is_change_point = false;
    // SAFETY: We checked that the event is valid. Lotto engine is single-threaded
    // so we know that event can't be modified by anyone else while this mutable reference exists.
    let event: &mut event_t = unsafe { event.as_mut() }.expect("Unexpected null pointer");
    let active_tids = &mut event.tset;
    let cat_table = &util_data.custom_cat_table;

    for handler in &util_data.arrival_or_execute_handlers_list {
        let handler = unsafe { &mut **handler };
        if !handler.enabled() {
            continue;
        }
        // SAFETY: We know that the Lotto engine is single-thread and so it wont be modified
        // while this mutable reference exists.
        let active_task_ids: Vec<TaskId> = unsafe { get_tids(active_tids) }
            .iter()
            .map(|x| TaskId::new(*x))
            .filter(|tid| !blocked_tasks.contains(tid))
            .collect();

        let cat_num: u32 = cat.0;
        let cat_end: u32 = base_category::CAT_END_.0;

        // check if it's a custom category
        if cat_num > cat_end {
            if let Some(cat_fn) = cat_table.get(&(cat_num)) {
                let custom_struct = cat_fn(ctx);
                custom_struct.call_right_handler(handler);
            }
        }
        let event_result: EventResult = handler.handle_arrival(tid, &ctx_info, &active_task_ids);
        let (cur_blocked_list, cur_is_chpt, cur_reason, read_only, cur_next_task_to_run) =
            event_result.to_tuple();
        if !cur_read_only {
            blocked_tasks.extend(cur_blocked_list);
            is_change_point |= cur_is_chpt;
            if reason.is_none() && cur_reason.is_some() {
                // we only update the reason if it is unset.
                reason = *cur_reason;
                debug!("Updating reason to {:?}", reason);
            }
            if cur_next_task_to_run.is_some() && next_task_to_run.is_none() {
                next_task_to_run = *cur_next_task_to_run;
            }
            if *read_only {
                cur_read_only = true;
            }
        }
    }

    table.insert(tid, ctx_info);
    trace!(
        "tid = {:?}, expected_to_run = {:?}, read_only = {:?}, reason = {:?}",
        tid,
        next_task_to_run,
        cur_read_only,
        reason
    );

    if received_read_only {
        debug!("Handler cant modify cappt because the event is read only.");
        return;
    }

    // We make sure to only update is_chpt from false->true or true->true.
    if is_change_point {
        event.is_chpt = true;
    }

    // We made sure that it was not read-only before, so we are not overwritting.
    if cur_read_only {
        event.readonly = true;
    }

    // Update reason. We ensure that we dont overwrite it by first checking if it is
    // not equal to the default.
    if event.reason == REASON_UNKNOWN
    /* un-set reason */
    {
        trace!("Setting reason {:?}", reason);
        match reason {
            Some(Reason::Shutdown(shutdown_reason)) => {
                event.reason = match shutdown_reason {
                    ReasonShutdown => REASON_SHUTDOWN,
                    ReasonSuccess => REASON_SUCCESS,
                    ReasonIgnore => REASON_IGNORE,
                };
            }
            Some(Reason::Abort(abort_reason)) => {
                event.reason = match abort_reason {
                    ReasonAssertFail => REASON_ASSERT_FAIL,
                    ReasonResourceDeadlock => REASON_RSRC_DEADLOCK,
                    ReasonSegfault => REASON_SEGFAULT,
                    ReasonSigint => REASON_SIGINT,
                    ReasonSigabrt => REASON_SIGABRT,
                    ReasonAbort => REASON_ABORT,
                    ReasonRutimeSegfault => REASON_RUNTIME_SEGFAULT,
                    ReasonRuntimeSigint => REASON_RUNTIME_SIGINT,
                    ReasonRuntimeSigabrt => REASON_RUNTIME_SIGABRT,
                }
            }
            Some(Reason::Deterministic) => {
                event.reason = REASON_DETERMINISTIC;
            }
            None => { /* ignore */ }
        }
    } else {
        debug!("Reason is already {:?}", event.reason);
        match reason {
            Some(Reason::Shutdown(_)) | Some(Reason::Abort(_)) => {
                warn!("Failed to shutdown...");
            }
            _ => { /* ok */ }
        }
    }

    // update active tids.
    let active_tids = &mut event.tset;
    blocked_tasks.sort();
    blocked_tasks.dedup();

    for task in &blocked_tasks {
        let task = u64::from(*task);
        trace!("Removing task {task:?} from tidset");
        // SAFETY: The caller promises us exclusive access to `event`
        unsafe {
            tidset_remove(active_tids as *mut tidset_t, task);
            debug!(
                "Active task ids after execute at arrrival handler: {:?}",
                get_tids(active_tids)
            );
        }
    }
    // TODO: check if it is already change_point
    // Limit the amount of events in the graph that we care here.
    if let Some(tid) = next_task_to_run {
        if get_tids(active_tids).len() > 1 {
            // SAFETY: The caller promised us that the `next_task_to_run` is a valid
            // tid and that it is containg in the `tset`.
            debug!("Current set: {:?}, tid = {:?}", get_tids(active_tids), tid);
            move_tid_to_beggining_of_tset(active_tids as *mut tidset_t, tid);
            event.selector = selector_SELECTOR_FIRST;
            event.is_chpt = true;
            assert!(!blocked_tasks.contains(&tid));
            assert!(event.readonly); // you must make this read_only!
        }
    }
}

/// Add's the handler to the subscribed list of `execute events`.
/// This requires passing the ownership of the handler.
pub fn subscribe_execute<Handler>(handler: &'static Handler)
where
    Handler: crate::engine::handler::ExecuteHandler + Send + Sync,
{
    let util_data = &mut crate::engine::handler::ENGINE_DATA
        .try_lock()
        .expect("single threaded");
    util_data
        .execute_handlers_list
        .push(handler as *const DynExecuteHandler as *mut _);
}

/// Add's the handler to the subscribed list of the arrival
/// `and` of the execute events.
/// This requires passing the ownership of the handler.
pub fn subscribe_arrival_or_execute<Handler>(handler: &'static Handler)
where
    Handler: crate::engine::handler::ArrivalOrExecuteHandler + Send + Sync,
{
    let util_data = &mut crate::engine::handler::ENGINE_DATA
        .try_lock()
        .expect("single threaded");
    util_data
        .arrival_or_execute_handlers_list
        .push(handler as *const DynArrivalOrExecuteHandler as *mut _);
}

/// Interface for adding a callback function for the rusty handlers
/// It gets the task-id of the task to run `now` directly from the
/// Sequencer Resume. This means that the decision is `final`
///
/// # Safety
///
/// This function is unsafe because it retrieves the context from
/// a c_void pointer. The user must ensure that it is a valid,
/// aligned and non-null pointer
///
pub unsafe extern "C" fn publish_execute(
    _chain: raw::chain_id,
    _type_: raw::type_id,
    event: *mut ::std::os::raw::c_void,
    _md: *mut raw::metadata,
) -> raw::ps_err {
    let ctx = get_context_from_publisher(&event);
    let util_data = &mut crate::engine::handler::ENGINE_DATA
        .try_lock()
        .expect("single threaded");

    let table = &mut crate::engine::handler::TASK_EVENT_TABLE
        .try_lock()
        .expect("single threaded");

    let tid = TaskId::new(ctx.id);
    if let Some(ctx_info) = table.remove(&tid) {
        trace!("Running Tid = {:?}", tid);
        for handler in &util_data.execute_handlers_list {
            let handler = unsafe { &mut **handler };
            if handler.enabled() {
                handler.handle_execute(tid, &ctx_info);
            }
        }
        for handler in &util_data.arrival_or_execute_handlers_list {
            let handler = unsafe { &mut **handler };
            if handler.enabled() {
                handler.handle_execute(tid, &ctx_info);
            }
        }
    } else {
        trace!("Running first event of tid = {:?}", tid);
        let ctx_info = ContextInfo::empty_context();
        for handler in &util_data.execute_handlers_list {
            let handler = unsafe { &mut **handler };
            if handler.enabled() {
                handler.handle_execute(tid, &ctx_info);
            }
        }
        for handler in &util_data.arrival_or_execute_handlers_list {
            let handler = unsafe { &mut **handler };
            if handler.enabled() {
                handler.handle_execute(tid, &ctx_info);
            }
        }
    }
    raw::ps_err_PS_OK
}

/// Register the capture point handler and subscribe to `TOPIC_NEXT_TASK`.
pub fn init() {
    use raw::slot_t::SLOT_RUSTY_ENGINE;
    trace!("Hello this is the lotto rusty pubsub handler initializing");
    // Safety: We assume no other function will be calling a dispacth for the same slot.
    unsafe {
        dispatcher_register(SLOT_RUSTY_ENGINE, Some(publish_arrival));
    }
    {
        // Subscribes to call from the sequencer_resume with the task-id to run.
        use raw::ps_subscribe;

        let args = std::ptr::null_mut::<::std::os::raw::c_void>();

        // Safety: We assume that the pubsub will not modify the args in any way
        // or try to read from it.
        unsafe {
            ps_subscribe(
                lotto_sys::CHAIN_LOTTO as u16,
                lotto_sys::TOPIC_NEXT_TASK as u16,
                Some(publish_execute),
                lotto_sys::DICE_MODULE_SLOT as i32,
            );
        }
    }
}

/// Get the task ids as a Vec from a tidset.
///
/// # Safety
///
/// The caller promises that `tset` is a valid, initialized and aligned `tidset_t`.
unsafe fn get_tids(tset: *const tidset_t) -> Vec<task_id> {
    assert!(!tset.is_null());
    assert_eq!(
        (tset as usize) % align_of::<tidset_t>(),
        0,
        "Invalid alignment"
    );
    let mut tids = vec![];
    let sz = unsafe { (*tset).size };
    for i in 0..sz {
        // SAFETY: We assume `tset` is valid, i.e. tasks points to an array of at least `sz` size.
        let id = unsafe { *(*tset).tasks.add(i) };
        tids.push(id);
    }
    tids
}

/// Moves `tid` to the beggining of the tset
///
/// # Safety
///
/// The caller promises that `tset` is a valid, initialized and aligned `tidset_t`.
/// The caller also ensures that `tid` is present in `tset`
unsafe fn move_tid_to_beggining_of_tset(tset: *mut tidset_t, tid: TaskId) {
    assert!(!tset.is_null());
    assert_eq!(
        (tset as usize) % align_of::<tidset_t>(),
        0,
        "Invalid alignment"
    );
    let sz = unsafe { (*tset).size };
    let mut pos: Option<usize> = None;
    for i in 0..sz {
        // SAFETY: We assume `tset` is valid, i.e. tasks points to an array of at least `sz` size.
        let id = unsafe { *(*tset).tasks.add(i) };
        if tid == TaskId::new(id) {
            pos = Some(i);
            break;
        }
    }
    // swap tset[0] and tset[pos]
    if let Some(pos) = pos {
        // SAFETY: We already checked that `tset` is valid for this indexes.
        let id_0 = unsafe { *(*tset).tasks.add(0) };
        let id_p = unsafe { *(*tset).tasks.add(pos) };
        debug!("Swapping {id_0} and {id_p}");
        *(*tset).tasks.add(0) = id_p;
        *(*tset).tasks.add(pos) = id_0;
    } else {
        trace!(
            "tid = {:?} not found in the `tset`. Unable to force it to run next",
            tid
        );
    }
}

/// Get the context_t from a ANY c_void pointer
///
/// # Safety
///
/// This function is unsafe because we get the context from a c_void pointer
/// The user must ensure that the pointer is alligned, not null and
/// contains a valid context_t.
///
unsafe fn get_context_from_publisher(v: &*mut ::std::os::raw::c_void) -> &context_t {
    let ctx_ptr: *const context_t = *v as *const context_t; // type cast to custom struct pointer
    assert_eq!(
        (ctx_ptr as usize) % align_of::<context_t>(),
        0,
        "Invalid alignment"
    );
    // SAFETY: We checked that the pointer is alligned and we expected the
    // user to send us a non-null, valid pointer
    let ctx: &context_t = unsafe { ctx_ptr.as_ref() }.expect("Unexpected null pointer");
    ctx
}

/// Guard against incorrect use of states.
static DURING_DISPATCH: AtomicBool = AtomicBool::new(false);

struct DispatchGuard;

impl DispatchGuard {
    fn enter() -> DispatchGuard {
        assert!(!DURING_DISPATCH.load(Ordering::SeqCst));
        DURING_DISPATCH.store(true, Ordering::SeqCst);
        DispatchGuard
    }
}

impl Drop for DispatchGuard {
    fn drop(&mut self) {
        DURING_DISPATCH.store(false, Ordering::SeqCst);
    }
}

pub fn during_dispatch() -> bool {
    DURING_DISPATCH.load(Ordering::SeqCst)
}
