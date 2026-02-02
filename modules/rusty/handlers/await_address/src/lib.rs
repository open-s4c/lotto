//! A lotto handler for waiting on changes to a variable address
//!
//! Currently only addresses of primitive types are supported and aligned accesses are assumed
//! Future enhancements could include watching for changes on an address range.
//! If `Any` of the addresses change then the task is unblocked.
//!
//! Potentially the race handler could be integrated with this handler in the future

use as_any::Downcast;
use category_macro::CategoryMacro;
use lotto::collections::FxHashMap;
use lotto::engine::handler::EventResult;
use lotto::engine::handler::{Address, ContextInfo, TaskId};
use lotto::engine::handler::{ExecuteArrivalHandler, ExecuteHandler};
use lotto::engine::pubsub::CustomCatTrait;
use lotto::log::{debug, trace, warn};
use lotto::raw::task_id;
use std::num::NonZeroUsize;
use std::sync::LazyLock;

static HANDLER: LazyLock<SwitchWaitingThread> = LazyLock::new(SwitchWaitingThread::new);

#[derive(Debug)]
struct InternalState {
    /// A Hashmap the address a task is blocked on for quick lookup if a task is blocked
    /// Is a map to prepare for watching multiple addresses in the future.
    blocked_tasks: FxHashMap<task_id, NonZeroUsize>,
    /// Reversed map for quick lookup if an address is monitored
    monitored_addresses: FxHashMap<NonZeroUsize, Vec<task_id>>,
    is_chpt: bool,
}

impl InternalState {
    fn new() -> Self {
        InternalState {
            blocked_tasks: FxHashMap::default(),
            monitored_addresses: FxHashMap::default(),
            is_chpt: false,
        }
    }
}

#[derive(CategoryMacro)]
#[handler_type(SwitchWaitingThread)]
pub struct Await {
    pub name: *const i8,
    address: Address,
    tid: TaskId,
}

impl AwaitContextHandler for SwitchWaitingThread {
    fn handle_await_context(&mut self, await_context: &Await) {
        self.block_cur_task_on_addr(
            u64::from(await_context.tid),
            NonZeroUsize::from(await_context.address),
        );
        // The current thread should be blocked, so this is always a change-point.
        self.state.is_chpt = true;
    }
}

struct SwitchWaitingThread {
    state: InternalState,
}

impl SwitchWaitingThread {
    fn block_cur_task_on_addr(&mut self, current_task_id: task_id, addr: NonZeroUsize) {
        let blocked_tasks = &mut self.state.blocked_tasks;
        let old = blocked_tasks.insert(current_task_id, addr);
        if let Some(old_addr) = old {
            if old_addr == addr {
                debug!("Task {current_task_id} was already marked as blocked on addr {addr:#x}");
                return;
            } else {
                warn!(
                    "Task {current_task_id} was supposed to wait for a write on `{old_addr:#x}`\
                    but the task progressed anyway and is now marked as waiting on addr `{addr:#x}`"
                );
            }
        }
        trace!("New blocked task {current_task_id} - Waiting on changes to {addr:#x}");

        if let Some(waiting_tasks) = self.state.monitored_addresses.get_mut(&addr) {
            debug_assert!(!waiting_tasks.contains(&current_task_id));
            waiting_tasks.push(current_task_id);
        } else {
            self.state
                .monitored_addresses
                .insert(addr, vec![current_task_id]);
        }
    }

    fn unblock_addr_now(&mut self, addr: NonZeroUsize) -> Vec<task_id> {
        if let Some(waiting_tasks) = self.state.monitored_addresses.remove(&addr) {
            // unblock tasks
            for task in &waiting_tasks {
                // Todo: if we watch multiple addresses per task, then we would need to remove those
                //       from the monitored addresses again (but without unblocking any new tasks)
                let _ = self
                    .state
                    .blocked_tasks
                    .remove(task)
                    .expect("Internal tables not in sync");
            }
            waiting_tasks
        } else {
            Vec::new()
        }
    }
    fn new() -> Self {
        Self {
            state: InternalState::new(),
        }
    }
}

impl ExecuteArrivalHandler for SwitchWaitingThread {
    fn handle_arrival(
        &mut self,
        _tid: TaskId,
        _ctx_info: &ContextInfo,
        _active_tids: &Vec<TaskId>,
    ) -> lotto::engine::handler::EventResult {
        trace!("Entered await_address_handle.");
        // todo: Check if the watchdog triggered and potentially unblock tasks, which could may
        //     have been blocked by an errounous user annotation.

        let blocked_tasks: Vec<TaskId> = self
            .state
            .blocked_tasks
            .keys()
            .map(|tid| TaskId::new(*tid))
            .collect();
        debug!("Await address handler will remove the following task ids: {blocked_tasks:?}");

        EventResult::new(blocked_tasks, self.state.is_chpt)
    }
}

impl ExecuteHandler for SwitchWaitingThread {
    fn handle_execute(&mut self, _tid: TaskId, ctx_info: &ContextInfo) {
        use lotto::engine::handler::AddressCatEvent::*;
        use lotto::engine::handler::AddressTwoValuesCatEvent::*;
        use lotto::engine::handler::AddressValueCatEvent::*;
        use lotto::engine::handler::EventArgs::*;

        match ctx_info.event_args {
            AddressOnly(BeforeWrite, addr, _) => {
                self.unblock_addr_now(NonZeroUsize::from(addr));
            }
            AddressValue(BeforeAWrite, addr, _, _) | AddressValue(BeforeRMW, addr, _, _) => {
                self.unblock_addr_now(NonZeroUsize::from(addr));
            }
            AddressTwoValuesEvent(BeforeCMPXCHG, addr, _, _, _) => {
                self.unblock_addr_now(NonZeroUsize::from(addr));
            }
            _ => { /* nothing or done at arrival */ }
        }
    }
}

pub fn register() {
    trace!("Hello this is the lotto await address handler written in Rust initializing");
    {
        let _ = await_cat();
        let await_cat_num = await_cat().0;
        let table = &mut lotto::engine::handler::ENGINE_DATA
            .try_lock()
            .expect("single threaded")
            .custom_cat_table;
        table.insert(await_cat_num, parse_await_context());
    }
    lotto::engine::pubsub::subscribe_arrival_or_execute(&*HANDLER);
}
