
#![allow(clippy::doc_lazy_continuation)]
#![allow(clippy::ptr_arg)]

//! A lotto handler that implements TruSt for SC memory model.
//!
//! Reference: https://plv.mpi-sws.org/genmc/
//! Tool-paper: https://plv.mpi-sws.org/genmc/cav21-paper.pdf
//! Algorithm: https://plv.mpi-sws.org/genmc/popl2022-trust.pdf
//!
//! Pros of using this instead of GenMC:
//! - GenMC uses a LLVM interpreter that makes it unable to work with certain
//! types of operations.
//! - Use other lotto features, eg. atomic/ignore blocks
//! - Easier to debug and possibility to replay.
//!
//! Cons of using this handler:
//! - It is much slower and with a bigger overhead than GenMC.
//! - It is currently only set for Sequential-Consistency Memory Model.
//!

use event_utils::{EventInfo, EventType};
use helpers::file_helper;
use lotto::base::envvar;
use lotto::base::Value;
use lotto::brokers::statemgr::*;
use lotto::cli::flags::*;
use lotto::cli::FlagKey;
use lotto::collections::{FxHashMap, FxHashSet};
use lotto::engine::handler::ContextInfo;
use lotto::engine::handler::Reason;
use lotto::engine::handler::ShutdownReason::ReasonShutdown;
use lotto::engine::handler::TaskId;
use lotto::engine::handler::{AddrSize, Address, ValuesTypes};
use lotto::engine::handler::{ExecuteArrivalHandler, ExecuteHandler};
use lotto::log::{debug, info, trace, warn};
use lotto::raw;
use lotto::Stateful;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::LazyLock;
use trust_algorithm::TruST;
use trust_algorithm::TrustAction;
use trust_algorithm::TrustAction::*;

pub mod helpers;
#[cfg(test)]
mod trust_tests;

static HANDLER: LazyLock<TrustHandler> = LazyLock::new(TrustHandler::new);

/// # Overview
///
/// Handler for integrating the TruST algorithm with lotto.
///
/// ## Saving states
/// It uses a file to save the state between runs.
/// We could instead save it to the trace.
/// `file` is by default `{temp-dir}/trust_file.mmap`.
///
/// ## Dealing with ASLR
///
/// To deal with the randomization of addresses, we need
/// to keep for every event in the stack a equivalence class
/// for the addresses. When we add a event in reconstruction
/// we already this address->symbol relation to a hashmap.
/// For new events, if the address is not in this symbol relation
/// we must give it a new ID and add it to the hashmap.
///
/// We could also keep the initial value of each symbol cached,
/// and a count of how many times each symbol appears, and remove
/// it from the cache if it doesnt appear anymore.
/// We dont do this because this would only be needed for CAS, and
/// we can avoid this by already inserting it in the stack with
/// the correct value of `is_rmw` (fail or now). (We actually need
/// for the cases when we are updating an iterator edge direction)
///
#[derive(Stateful)]
struct TrustHandler {
    trust: TruST,
    /// Used for a sanity check, to see if the engine is
    /// respecting the handler decision
    expected_next_tid: Option<TaskId>,
    /// Contains the file were the state of the handler is
    /// saved between runs.
    /// The default value should be altered ONLY FOR DEBUG.
    file: String,
    /// Contains all the threads that appeared and have not issued a
    /// CAT_TASK_FINI yet. As soon as the TASK_FINI appears on `arrival`,
    /// the thread is removed, as it should never execute again.
    active_threads: FxHashSet<TaskId>,
    event_info: FxHashMap<TaskId, EventInfo>,
    /// We also need to keep the addres and expected value,
    /// because for CAS we need to check if it is a `rmw` when we are
    /// adding it, not when we receive the event.
    cas_info: FxHashMap<TaskId, (Address, AddrSize, ValuesTypes)>,
    /// In case we tried to shutdown but was refused, we save
    /// the previous intent and keep retrying.
    ///
    /// This can happen in case of a read_only event or if
    /// another handler already wrote something in reason.
    previous_event_shutdow_reason: Option<Reason>,
    // Extra parameters:
    /// Only on for unit tests.
    test_mode: bool,
    initialized: bool,

    /// Config state
    #[config]
    cfg: Config,
}

// Safety: single thread.
unsafe impl Sync for TrustHandler {}

impl TrustHandler {
    fn new() -> Self {
        Self {
            trust: TruST::new(),
            expected_next_tid: None,
            file: "trust_file.mmap".to_string(),
            active_threads: FxHashSet::default(),
            event_info: FxHashMap::default(),
            cas_info: FxHashMap::default(),
            previous_event_shutdow_reason: None,
            test_mode: false,
            initialized: false,
            cfg: Config {
                // NOTE: The default value must be true. This handler
                // needs to set up some internal states VERY EARLY.
                enabled: AtomicBool::new(true),
            },
        }
    }

    /// Returns `true` if the handler is enabled
    fn init_handler_and_check_if_enabled(&mut self) -> bool {
        self.initialized = true;
        if !self.cfg.enabled.load(Ordering::Relaxed) {
            trace!(
                "Lotto-TruSt not set. To use it, use the CLI flag 
                `handler-trust enable`. Read the README for more details"
            );
            // return default value
            return false;
        }
        let dir = envvar::must_get("LOTTO_TEMP_DIR");
        // Load content from previous run
        let full_path = dir + "/trust_file.mmap";
        self.set_file_name(&full_path);
        self.intialize_content_from_previous_run();
        true
    }
    /// Change the file name used for saving data between run.
    /// Should be relative to the temporary directory.
    fn set_file_name(&mut self, file: &String) {
        self.file = file.clone();
    }
}

use lotto::engine::handler::EventResult;
impl ExecuteArrivalHandler for TrustHandler {
    fn handle_arrival(
        &mut self,
        tid: TaskId,
        ctx_info: &ContextInfo,
        active_tids: &Vec<TaskId>,
    ) -> EventResult {
        if !self.initialized && !self.init_handler_and_check_if_enabled() {
            return EventResult::new(Vec::new(), false);
        }
        let program_about_to_finish = tid == TaskId::new(1)
            && ctx_info.cat == lotto::raw::base_category::CAT_FUNC_EXIT
            && self.active_threads.len() == 1;

        if let Some(prv_reason) = self.previous_event_shutdow_reason {
            let mut event_result = EventResult::new(Vec::new(), false);
            event_result.set_reason(prv_reason);
            if program_about_to_finish {
                panic!(
                    "Lotto TruSt denied finishing. To avoid unexpected behavior we finish.
                The total number of execution graph explored was {:?}",
                    self.trust.get_total_number_of_graphs()
                );
            }
            return event_result;
        }
        self.update_event_of_tid(tid, ctx_info, program_about_to_finish);
        self.update_active_threads_arrival(tid, ctx_info);

        if self.no_relevant_events(active_tids) {
            // If no event is a read, write, End or any other important event, we
            // skip to avoid extra change points.
            self.expected_next_tid = None; // any is good
            return EventResult::new(/*blocked_tasks*/ Vec::new(), false);
        }

        let (tid, ev_info) = self.next_thread_to_run(active_tids);
        let tid: u64 = u64::from(tid);
        self.update_init_value(&ev_info);

        let result = self.trust.handle_new_event(tid as usize, ev_info);
        debug!(
            "Next TruSt action is {:?}, tid = {:?}, active_tids = {:?}, ev_info = {:?}",
            result, tid, &active_tids, &ev_info
        );

        // Decide next handler move & return accordinly
        self.decide_next_handler_action(tid, result)
    }
}

impl TrustHandler {
    /// Decides the next thread to run, or request for a program
    /// shutdown (i.e. rebuild) or a program abort (i.e. every program explored)
    fn decide_next_handler_action(&mut self, tid: u64, result: TrustAction) -> EventResult {
        let reason = match result {
            Add => {
                // If there is a corresponding RMW-write, we must check for it as well.
                // We have to do it here because when we request lotto to run this thread
                // it will execute BOTH the read and write at once.
                // For rebuilding, we just add all of them.
                self.add_prefix_of_unmatched_rmw_writes_in_rebuild();
                if self.trust.expects_rmw_write() {
                    debug!("Attempt RMW write from {:?}", tid);
                    self.add_rmw_write_event()
                } else {
                    self.expected_next_tid = Some(TaskId::new(tid));
                    Some(Reason::Deterministic)
                }
            }
            Rebuild => {
                debug!("TruSt handler is about to rebuild");
                // now that we decided to shutdown, we need to save this to a file
                self.save_state_to_file();
                // we shutdown so there is no next expected thread.
                self.expected_next_tid = None;
                Some(Reason::Shutdown(ReasonShutdown))
            }
            End => {
                // All the graphs were explored successfully!
                self.expected_next_tid = Some(TaskId::new(tid));
                // We must use a Abort reason. Unfortunatly this comes with the
                // side-effect of a non-zero exit code.
                // TODO: Considering adding a new CLI command for this handler and
                // a "AbortSuccess" reason for it.
                file_helper::delete_file(&self.file);
                self.print_total_number_of_graphs();
                Some(Reason::Abort(
                    lotto::engine::handler::AbortReason::ReasonAbort,
                ))
            }
        };

        let event_result = EventResult::new_from_schedule(self.expected_next_tid, reason);
        // We assert that the previous reason is none, because it shouldn't be allowed
        // to be running if it isnt.
        assert!(self.previous_event_shutdow_reason.is_none());

        if reason.is_some() && Reason::is_shutdown_or_abort(&reason.unwrap()) {
            self.previous_event_shutdow_reason = reason;
            debug!("TruSt handler is trying to shutdown");
        }
        event_result
    }
}

impl ExecuteHandler for TrustHandler {
    fn enabled(&self) -> bool {
        self.cfg.enabled.load(Ordering::Relaxed)
    }
    fn handle_execute(&mut self, tid: TaskId, ctx_info: &ContextInfo) {
        // note: the first of all the events happens before initialiazing
        // trust, so be carefull when adding stuff here.
        trace!(
            "Executing {:?} and operation {:?} in TruSt handler",
            tid,
            ctx_info.cat
        );
        trace!("Expecting {:?}", self.expected_next_tid);
        // We check if tid is really the expected value.
        assert!(
            !self.active_threads.contains(&tid) // first event of thread
            || self.expected_next_tid.is_none() || self.expected_next_tid == Some(tid)
        );
        if self.active_threads.contains(&tid) {
            self.event_info.remove(&tid);
        }
        self.update_active_threads_execute(tid, ctx_info);
    }
}

impl TrustHandler {
    fn intialize_content_from_previous_run(&mut self) {
        match file_helper::read_single_lined_file(&self.file) {
            Some(content) => {
                self.trust = TruST::deserialize_and_init(content);
                debug!("Successfuly loaded file from previous run");
            }
            None => {
                // file doesnt exist or we were unable to open it
                // This should only happen for the first run.
                warn!("Unable to open file {:?}", &self.file);
            }
        }
    }

    fn update_init_value(&mut self, ev_info: &EventInfo) {
        if let Some(addr) = ev_info.addr {
            // For test modes we cant access the value in an adress.
            if !self.test_mode {
                // TODO: When adding mixed-size operations we probably want to load
                // more than just this address.
                let length = ev_info.addr_length.unwrap();
                let val = lotto::engine::handler::get_value_from_addr(&addr, &length);
                self.trust.update_init_value_if_not_set(addr, val);
            } else {
                self.trust
                    .update_init_value_if_not_set(addr, ValuesTypes::U32(0));
            }
        }
    }

    fn update_event_of_tid(
        &mut self,
        tid: TaskId,
        ctx_info: &ContextInfo,
        program_about_to_finish: bool,
    ) {
        let ev_info = if program_about_to_finish {
            EventInfo::end()
        } else {
            EventInfo::new_from_context(ctx_info)
        };

        if EventInfo::is_cas(&ev_info) {
            let (addr, addr_size, expected_value) =
                ctx_info.get_cas_addr_and_expected_value().unwrap();
            self.cas_info.insert(tid, (addr, addr_size, expected_value));
        } else {
            self.cas_info.remove(&tid);
        }

        let old_ev_info = self.event_info.insert(tid, ev_info);
        assert!(old_ev_info.is_none()); // only should have 1 event at a time.
        if ev_info.is_rmw {
            trace!("Arrival of RMW from {:?}, {:?}", tid, ev_info);
        }
    }
    fn add_rmw_write_event(&mut self) -> Option<Reason> {
        let (tid, addr, length, val) = self.trust.get_next_rmw_write();
        let tid = u64::from(tid) as usize;
        let result_rmw = self.trust.handle_new_event(
            tid,
            EventInfo::new(
                0,
                Some(addr),
                Some(length),
                EventType::Write,
                true,
                true,
                val,
            ),
        );
        match result_rmw {
            Add => {
                // Ok, we are already adding
                self.expected_next_tid = Some(TaskId::new(tid as u64));
                Some(Reason::Deterministic)
            }
            Rebuild => {
                // we will decide to shutdown, so we need to save this to a file
                self.save_state_to_file();
                // we shutdown so there is no next thread.
                self.expected_next_tid = None;
                Some(Reason::Shutdown(ReasonShutdown))
            }
            End => panic!("Unexpected trustAction {:?}", result_rmw),
        }
    }

    /// If `CAT_TASK_FINI` appears, the thread is removed from the set.
    /// It is not added in this function, but on `update_active_threads_execute`.
    fn update_active_threads_arrival(&mut self, tid: TaskId, ctx_info: &ContextInfo) {
        match ctx_info.cat {
            lotto::raw::base_category::CAT_TASK_FINI => {
                if self.active_threads.contains(&tid) {
                    debug!("Removing from the active threads set: {:?}.", tid);
                    self.active_threads.remove(&tid);
                }
            }
            _ => { /* ignore */ }
        }
    }

    /// If this is the first event of thread it will add the tid to
    /// the set of active threads.
    /// The threads are not removed on this function, but on `update_active_threads_execute`.
    fn update_active_threads_execute(&mut self, tid: TaskId, _: &ContextInfo) {
        if !self.active_threads.contains(&tid) {
            // first event of thread
            debug!("Addding to the active threads set: {:?}", tid);
            self.active_threads.insert(tid);
        }
    }

    fn print_total_number_of_graphs(&self) {
        println!(
            "Total number of execution graphs: {:?}",
            self.trust.get_total_number_of_graphs()
        );
    }

    /// Returns `true` if all the events of all active threads
    /// are equal to `None`.
    ///
    /// `active_tids`: set of active threads passed by the lotto engine.
    fn no_relevant_events(&self, active_tids: &Vec<TaskId>) -> bool {
        active_tids.iter().all(|tid| {
            let ev_info = &self.event_info[tid];
            ev_info.event_type == EventType::None
        })
    }

    /// TODO: fix the case where task 1 calls other functions other then main...
    /// This may trigger as a final event but isnt.
    /// If we were using the trace instead of a file this would not be a issue.
    fn next_thread_to_run(&mut self, active_tids: &Vec<TaskId>) -> (TaskId, EventInfo) {
        // If we are reconstructing:
        if self.trust.is_rebuilding() {
            return self.next_thread_in_rebuild(active_tids);
        }
        // get the thread with minimum id which is active.
        // We unwrap because if the active_tids is empty, we would have
        // set ANY_TASK to run beforehand.
        // [TODO]: To reduce the number of breakpoints, we should try to get the minimum ID that
        // has a "relevant" event. Else we can let ANY_TASK run. For some reason adding this to
        // the `filter` directly does not work. I dont know why, but it may have something to do
        // with a CAT_CALL (like pthread_create/join), that makes a thread be unavailable for
        // some time, because when I tried this optimization it would fail right after it.
        let task_1 = TaskId::new(1);
        let mn_thread = match active_tids.iter().filter(|&x| x != &task_1).min() {
            None => {
                // no active task besides 1.
                // We do this as a temporary fix issues with pthread_create/join
                // Mainly the issue is that for `create` we may execute a thread
                // that is not in the active_thread. In `join` the event is deleted
                // from `active_tids` before the handle_arrival, but it will still execute
                // We must remove this fix by handling create/join as special cases.
                assert!(self.active_threads.contains(&task_1));
                task_1
            }
            Some(t) => *t,
        };

        trace!("Normally chosen");
        let mut ev_info = self.event_info[&mn_thread];
        if let Some((addr, lenght, expected)) = self.cas_info.remove(&mn_thread) {
            ev_info.is_rmw =
                lotto::engine::handler::get_value_from_addr(&addr, &lenght) == expected;
        }
        (mn_thread, ev_info)
    }

    fn next_thread_in_rebuild(&mut self, active_tids: &Vec<TaskId>) -> (TaskId, EventInfo) {
        let (tid, ev_info) = self.trust.get_next_trace_event();
        debug!("Reconstruction. We want to run {:?}, {:?}", tid, ev_info);
        if !active_tids.contains(&tid) {
            // TODO: Fix the issue below to allow other threads creating new threads.
            // If the task doesnt exist we run task 1 until it does...
            // This is a workaround that assumes that only thread 1 can create threads.
            // The nicer approach would be to save the CAT_TASK_CREATE events in the trace
            // as well.
            debug!(
                "Running thread 1 because we didnt find the right one. Expected was {:?} {:?}",
                tid, ev_info
            );
            let task_1 = TaskId::new(1);
            let mut ev_info = self.event_info[&task_1];

            ev_info.event_type = EventType::None; // Ignore
            return (task_1, ev_info);
        }
        let mut nxt_ev_info = self.event_info[&tid];

        if let Some((addr, lenght, expected)) = self.cas_info.remove(&tid) {
            nxt_ev_info.is_rmw =
                lotto::engine::handler::get_value_from_addr(&addr, &lenght) == expected;
        }

        if nxt_ev_info.event_type == ev_info.event_type {
            // with this we assert that the `None` events, which are not on the trace
            // are ignored
            self.assert_event_equals_expected_in_rebuild(&nxt_ev_info, &ev_info);
            debug!("expected: {:?}\n, cur: {:?}", ev_info, nxt_ev_info);
            // update insertion order from the trace:
            nxt_ev_info.insertion_order = ev_info.insertion_order;
        } else {
            // `None` events are not in the trace, so we can skip them.
            assert!(nxt_ev_info.event_type == EventType::None);
        }
        (tid, nxt_ev_info)
    }

    /// Adds all the following Exclusive writes in the trace directly.
    /// We do this here, because Lotto generates only one event for both the
    /// read or write (CAT_BEFORE_RMW/COMPARE_EXCHANGE).
    fn add_prefix_of_unmatched_rmw_writes_in_rebuild(&mut self) {
        while self.trust.is_rebuilding() {
            let (tid, ev_info) = self.trust.get_next_trace_event();
            if ev_info.event_type != EventType::Write || !ev_info.is_rmw {
                return;
            }
            debug!(
                "Adding RMW write in rebuild from {:?}. Ev_info = {:?}",
                &tid, &ev_info
            );
            let mut ev_info = ev_info;
            ev_info.addr = Some(
                self.trust
                    .get_real_addr_from_virtual(&ev_info.addr.unwrap()),
            );
            self.trust.handle_new_event(usize::from(tid), ev_info);
        }
    }

    fn save_state_to_file(&self) {
        let state = self.trust.serialize();
        file_helper::write_to_file(&self.file, &state);
    }

    // SANITY CHECKS
    fn assert_event_equals_expected_in_rebuild(
        &self,
        ev_info: &EventInfo,
        expec_ev_info: &EventInfo,
    ) {
        // Variables can have a different address due to random layouts,
        // so we dont do a sanity check on that.
        // 'is_rmw' can change, so we check only if atomicity is preserved.
        assert!(expec_ev_info.is_atomic == ev_info.is_atomic);
    }
}

#[derive(Encode, Decode, Debug, Default)]
struct Config {
    enabled: AtomicBool,
}

impl Marshable for Config {
    fn print(&self) {
        let enabled = self.enabled.load(Ordering::Relaxed);
        info!("enabled: {}", if enabled { "on" } else { "off " });
    }
}

pub fn register() {
    trace!("Registering Lotto TruSt handler, written in rust.");
    lotto::engine::pubsub::subscribe_arrival_or_execute(&*HANDLER);
    lotto::brokers::statemgr::register(&*HANDLER);
}

pub static FLAG_HANDLER_TRUST_ENABLED: FlagKey = FlagKey::new(
    c"HANDLER_TRUST_ENABLED",
    c"",
    c"handler-trust",
    c"",
    c"enable the TruSt handler",
    Value::Bool(false),
    &STR_CONVERTER_BOOL,
    Some(_handler_trust_enabled_callback),
);

pub fn register_flags() {
    let _ = FLAG_HANDLER_TRUST_ENABLED.get();
}

unsafe extern "C" fn _handler_trust_enabled_callback(v: raw::value, _: *mut std::ffi::c_void) {
    let enabled = Value::from(v).is_on();
    HANDLER.cfg.enabled.store(enabled, Ordering::Relaxed);
}
