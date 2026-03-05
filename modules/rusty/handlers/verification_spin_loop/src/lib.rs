#![allow(clippy::doc_lazy_continuation)]

//! A lotto handler for preventing iterations of user annotated spin loops
//! that should wait to make progress.
//!
//! Currently, only addresses of primitive types are supported
//! and aligned accesses are assumed.
//! Future enhancements could include watching for changes on an address range.
//! Read the documentation in `lotto/docs/api/` for usage details.
//!
//! # Algorithm explanation:
//!
//! Every spinloop can be identified by a tuple (tid, depth), that indicates
//! the task id and the depth of the spinloop, in case of nesting.
//! We keep for Every active spin_loop wheter it is "co-maximal" or not. That
//! means that `every` read, reads from the `latest` write.
//! We keep track of the following category of events:
//!
//! - Reads:
//!     For every active spin_loop we keep a set of all the reads. The read is
//! propagated from the inner loop to all others.
//!
//! - Writes:
//!     For every write, we see all the spin_loops that read
//! from this address, including from the same task, then mark them as
//! "not co-maximal", meaning they don't read the latest write of every
//! address. Note that we ignored the values of each write for simplicity,
//! so we could instead check if it is reading from the latest value. Note
//! that soundness is still ensured, meaning that if a task is blocked,
//! then it should be, but if it isn't blocked, maybe we could actually
//! block it if looking at the values. That way, we don't introduce deadlocks,
//! but there may be spurious wakeups.
//!
//! - SPIN_START:
//!     Initiate the data_structures for the spin and increase
//!     the loop depth if last operation was not a unsuccessful
//!     spin_end.
//!
//! - SPIN_END:
//!     Check if the task should be blocked, or if it should clean up the
//! data structure and subtract one from the loop-depth. If it is blocked,
//! the data structure is cleaned later.
//!
//!
//! # Special cases - Spin-loops with writes
//!
//! There are two special cases when we have a spin_loop with writes:
//!
//! - Case 1)
//! The loop reads and writes to the same address
//! For example,
//!     
//!     x = 0;
//!     while(x == 0)
//!         x = 1;
//!
//! If we block after one iteration it would be incorrect. But for:
//!
//! y = 0;
//! while(y==0){
//!     x = 0;
//!     y = x;
//!     x = 1;
//! }
//!
//! It is ok to block-it. In general, if there is a situation with Write -> Read -> Write, it is ok
//! If there is Read -> Write then we can't know, and, to ensure soundness, we better not block
//! We can deal with this case by creating a state machine for each loop with the following states:
//! Initial (I), Safe (S), ReadNeverWrote (R), Final State (F)
//! The transitions are:
//! Read: I -> R, S -> S, R -> R
//! Write: I -> S, S -> S , R -> F
//! In case of the state `F` we can actually already stop this process and remove the loop from
//! observation, because it will be unblocked for sure.
//!
//! - Case 2)
//! The loop, if it was to repeat, would write in another loop.
//! For example:
//!
//!     while(y == 0)           | x=0;
//!     x=1;                    | while(x==0);
//!                             | y = 1;
//!
//! The solution is to keep a multiset of all the addresses written by the innermost part
//! of each blocked spin loop. If a read reads from a write in this multiset, then it is not
//! co-maximal. But also, the loop that writes must also be released!
//!
//! # SAFETY
//!
//! We assume that the user will NOT use the annotations directly,
//! and instead will use the await_while and await_break `macros`.
//! We assume that the passed addresses and values from rusty_pubsub
//! are correct, safe and not NULL.
//! Note: A program can still have NULL addresses, but it can't be passed
//! to this handler.
//!

use as_any::Downcast;
use category_macro::CategoryMacro;
use lotto::base::Value;
use lotto::brokers::Marshable;
use lotto::brokers::{Decode, Encode};
use lotto::cli::flags::STR_CONVERTER_BOOL;
use lotto::cli::FlagKey;
use lotto::collections::FxHashMap;
use lotto::collections::FxHashSet;
use lotto::engine::handler::ContextInfo;
use lotto::engine::handler::{Address, ValuesTypes};
use lotto::engine::handler::{ExecuteArrivalHandler, ExecuteHandler};
use lotto::engine::pubsub::CustomCatTrait;
use lotto::log::{debug, info, trace};

use lotto::engine::handler::TaskId;

#[derive(Clone, Copy, PartialEq, Debug)]
enum AddressState {
    Initial,
    ReadNeverWrote,
    Safe,
    Final,
}

#[derive(Default)]
struct InternalState {
    /// Keeps the set of task-ids that can't make progress yet.
    blocked_tasks: FxHashSet<TaskId>,
    /// Keeps the number of nested spin-loops for each task
    /// If no active loop there is value for the key.
    loop_depth: FxHashMap<TaskId, u32>,
    /// For each Address keeps a vector with all the tuples of (tid, loop-depth)
    /// That are watching it.
    /// Note: In case the vector is slow due to O(N) removal, one could use a set instead
    monitoring_address: FxHashMap<Address, Vec<(TaskId, u32)>>,
    /// Keeps for each (tid,loop-depth) a vector with all
    /// the addresses it is looking for writes
    reading_addresses: FxHashMap<(TaskId, u32), Vec<Address>>,
    /// Keeps the state machine for writes in the same loop.
    /// For more details see the "Spin-loops with writes" section in
    /// the header.
    state_write_within_loop: FxHashMap<(TaskId, u32), FxHashMap<Address, AddressState>>,
    /// keeps all the loops that still are reading from comaximal write,
    /// That is, there was no in-between writes from other tasks after
    /// each read. Only for this tuples we care about monitoring reads and
    /// writes. If a task is already not comaximal, we ignore entries in the
    /// other hash maps.
    comaximal_loops: FxHashSet<(TaskId, u32)>,
    /// Is true if the last write changed the blocked list
    last_write_changed_blocked_list: bool,
    /// Keeps the number of writes to each address done by the blocked spin-loops
    /// This deals with the Special case of reading from a blocked loop.
    num_writes_by_blocked_loops: FxHashMap<Address, u32>,
    /// For each address, keeps a list of all the loops that, if re-run,
    /// would write on it.
    blocked_loops_writing_here: FxHashMap<Address, Vec<(TaskId, u32)>>,
    /// list of writes. Empty for non-blocked loops
    writes_in_blocked_loop: FxHashMap<(TaskId, u32), Vec<Address>>,
    /// keeps a set with all tasks that just had a spin_end(0). In this way
    /// The spin_start can know if it is a new one or just a loop iteration.
    last_action_was_failed_spin_end: FxHashSet<TaskId>,
    is_chpt: bool,
}

#[derive(CategoryMacro)]
#[handler_type(SpinLoopHandler)]
pub struct SpinStart {
    pub name: *const i8,
    tid: TaskId,
}

#[derive(CategoryMacro)]
#[handler_type(SpinLoopHandler)]
pub struct SpinEnd {
    pub name: *const i8,
    tid: TaskId,
    val: ValuesTypes,
}

impl SpinStartContextHandler for SpinLoopHandler {
    fn handle_spin_start_context(&mut self, spin_start_context: &SpinStart) {
        self.mark_spin_loop_begin(spin_start_context.tid);
    }
}

impl SpinEndContextHandler for SpinLoopHandler {
    fn handle_spin_end_context(&mut self, spin_end_context: &SpinEnd) {
        self.handler_state.is_chpt |=
            self.mark_spin_loop_end(spin_end_context.tid, &spin_end_context.val);
    }
}

#[derive(lotto::Stateful)]
pub struct SpinLoopHandler {
    handler_state: InternalState,
    #[config]
    cfg: SpinLoopHandlerConfig,
}

#[derive(Encode, Decode, Debug)]
pub struct SpinLoopHandlerConfig {
    pub enabled: AtomicBool,
}

impl Marshable for SpinLoopHandlerConfig {
    fn print(&self) {
        info!("enabled = {}", self.enabled.load(Ordering::Relaxed));
    }
}

impl SpinLoopHandler {
    pub fn new() -> Self {
        Self {
            handler_state: InternalState::default(),
            cfg: SpinLoopHandlerConfig {
                enabled: false.into(),
            },
        }
    }
    /// Clears the reading_address of the tuple (tid, depth) and removes it
    /// From all the writes it was monitoring.
    /// If the task is being unblocked, we also remove the writes from the
    /// `writes_in_blocked_loop` list
    ///
    /// SAFETY: Ensure the loop is not removed twice
    ///
    /// Returns true if a blocked task is unblocked
    fn remove_loop_from_observation(&mut self, tid: TaskId, depth: u32) -> bool {
        let loop_tuple = (tid, depth);
        let state = &mut self.handler_state;
        assert!(state.comaximal_loops.contains(&loop_tuple));
        state.comaximal_loops.remove(&loop_tuple);

        let reading_addresses = state
            .reading_addresses
            .get(&loop_tuple)
            .expect("non empty vec")
            .clone(); // TODO: use borrow_split to remove the clone

        // remove me from addresses I read from
        for addr in reading_addresses {
            let loops = state
                .monitoring_address
                .get_mut(&addr)
                .expect("The write must be monitoring this address");
            loops.retain(|&l| l != loop_tuple);
        }

        state.reading_addresses.remove(&(tid, depth));
        state.state_write_within_loop.remove(&(tid, depth));
        if *state.loop_depth.get(&tid).unwrap() == depth && state.blocked_tasks.remove(&tid) {
            debug!("Spin-loop handler unblocked {:?}", tid);
            // If the loop is being unblocked, update the the blocked_loops_write list
            // to remove it
            let write_list = state
                .writes_in_blocked_loop
                .get(&(tid, depth))
                .expect("non empty map")
                .clone(); // TODO: can remove this clone?
            for addr in write_list {
                let cur_cnt = state.num_writes_by_blocked_loops.entry(addr).or_insert(0);
                if *cur_cnt > 1 {
                    *cur_cnt -= 1;
                } else {
                    state.num_writes_by_blocked_loops.remove(&addr);
                }
                if let Some(vec) = state.blocked_loops_writing_here.get_mut(&addr) {
                    vec.retain(|l| *l != (tid, depth))
                }
            }
            state.writes_in_blocked_loop.remove(&(tid, depth));
            true
        } else {
            false
        }
    }

    /// Executes a write from cur_id into `addr`
    /// Unblocks every loop that reads from `addr`
    ///
    /// Return value: `true` if it is a change point, in this case,
    /// if some task is unblocked.
    fn execute_write_event(&mut self, cur_id: TaskId, addr: Address) -> bool {
        debug!("Write at {:?} now by id {:?}!\n", addr, cur_id);
        let mut is_change_point = false;
        if let Some(list_of_reading_loops) =
            self.handler_state.monitoring_address.get(&addr).cloned()
        {
            // this could be done faster because we remove from this same vector
            // all the events from different tasks each in O(n)
            // must clone because we remove from the vector in the loop when
            // calling the function.
            trace!(
                "List of tasks reading to this address: {:?}",
                list_of_reading_loops
            );
            for (tid, depth) in list_of_reading_loops {
                if tid != cur_id {
                    debug!(
                        "task {:?} depth {} notified of write to {:?} from task {:?}\n",
                        tid, depth, addr, cur_id
                    );
                    is_change_point |= self.remove_loop_from_observation(tid, depth)
                }
            }
        }
        // writes into the same loop:
        if let Some(cur_depth) = self.handler_state.loop_depth.get(&cur_id) {
            for depth in 1..=*cur_depth {
                is_change_point |= self.update_address_state_machine_for_write(cur_id, depth, addr);
            }
        }
        is_change_point
    }

    /// Update the state-machine of reads-write within the same loop
    /// for the case of a write.
    ///
    /// Return: `true` if it is a change point
    fn update_address_state_machine_for_write(
        &mut self,
        tid: TaskId,
        depth: u32,
        addr: Address,
    ) -> bool {
        if !self.handler_state.comaximal_loops.contains(&(tid, depth)) {
            return false;
        }
        let mut is_change_point = false;
        let cur_state = *self
            .handler_state
            .state_write_within_loop
            .get_mut(&(tid, depth))
            .expect("Non empty map")
            .entry(addr)
            .or_insert(AddressState::Initial);

        let next_state = {
            match cur_state {
                AddressState::Initial | AddressState::Safe => AddressState::Safe,
                AddressState::ReadNeverWrote => {
                    // to preserve soundeness, we must remove this from observation:
                    info!("Removed task {:?} after self-write\n", tid);
                    is_change_point |= self.remove_loop_from_observation(tid, depth);
                    AddressState::Final
                }
                AddressState::Final => {
                    panic!("Final state should remove loop from observation. Did you use the annotations with the macros?");
                }
            }
        };

        if next_state != AddressState::Final {
            let map = self
                .handler_state
                .state_write_within_loop
                .get_mut(&(tid, depth))
                .expect("Non empty map");
            map.insert(addr, next_state);
        }

        is_change_point
    }

    /// Adds to the set of monitoring address of the task this new read
    /// In case of nested loops, this is added for all the nestings
    fn add_read_to_all_active_loops_of_task(&mut self, tid: TaskId, addr: Address) {
        let cur_depth = self.handler_state.loop_depth.get(&tid);

        if let Some(cur_depth) = cur_depth {
            for depth in 1..cur_depth + 1 {
                if self.handler_state.comaximal_loops.contains(&(tid, depth)) {
                    if self
                        .handler_state
                        .num_writes_by_blocked_loops
                        .contains_key(&addr)
                    {
                        // at least one loop is constanly able to write here, so we can already
                        // remove this loop from observation!
                        info!("Reading from permanent write. Unblock task {:?}", tid);
                        self.remove_loop_from_observation(tid, depth);
                        for (w_id, w_depth) in self
                            .handler_state
                            .blocked_loops_writing_here
                            .get(&addr)
                            .expect("Not empty vec")
                            .clone()
                        {
                            self.remove_loop_from_observation(w_id, w_depth);
                        }
                        continue;
                    }

                    // must add this read and also watch for it:
                    let cur_reading = self
                        .handler_state
                        .reading_addresses
                        .entry((tid, depth))
                        .or_default();
                    if cur_reading.contains(&addr) {
                        continue;
                    }
                    cur_reading.push(addr);
                    let cur_watchlist = self
                        .handler_state
                        .monitoring_address
                        .entry(addr)
                        .or_default();
                    cur_watchlist.push((tid, depth));
                    debug!(
                        "Task {:?} depth {} is now reading from {:?}\n",
                        tid, depth, addr
                    );
                    self.update_address_state_machine_for_read(tid, depth, addr);
                }
            }
        }
    }

    /// Updates t he state machine for reads-writes within the same spin-loop
    fn update_address_state_machine_for_read(&mut self, tid: TaskId, depth: u32, addr: Address) {
        // Even for the address-states, we only care about the first read,
        // Because Initial->R and the rest of the states doesn't change
        let cur_map = self
            .handler_state
            .state_write_within_loop
            .get_mut(&(tid, depth))
            .expect("Non empty map");
        cur_map.entry(addr).or_insert(AddressState::ReadNeverWrote);
    }

    fn execute_read_event(&mut self, tid: TaskId, addr: Address) {
        self.add_read_to_all_active_loops_of_task(tid, addr);
    }

    fn mark_spin_loop_begin(&mut self, tid: TaskId) {
        let depth = self.handler_state.loop_depth.entry(tid).or_insert(0);

        if *depth == 0
            || !self
                .handler_state
                .last_action_was_failed_spin_end
                .contains(&tid)
        {
            *depth += 1;
        } else {
            self.handler_state
                .last_action_was_failed_spin_end
                .remove(&tid);
        }

        let depth = *self
            .handler_state
            .loop_depth
            .get(&tid)
            .expect("Must have at least a loop_begin before a spin_start");

        debug!("Starting spin loop for {:?} and depth {depth}.\n", tid);

        // We can force unblock this task if needed
        // because in case there are no active tasks we can
        // schedule a blocked one, or in case of a read-only event.
        if self.handler_state.blocked_tasks.contains(&tid) {
            // For now we simply unblock.
            // Other options are to test liveness, but it can schedule a blocked task
            // instead of doing pthread_create for example..
            self.remove_loop_from_observation(tid, depth);
            info!("spurious awake to task {tid:?}");
        }

        assert!(!self.handler_state.blocked_tasks.contains(&tid));
        assert!(!self
            .handler_state
            .reading_addresses
            .contains_key(&(tid, depth)));
        // make sure that the task is put in a comaximal state
        assert!(!self.handler_state.comaximal_loops.contains(&(tid, depth)));
        assert!(!self
            .handler_state
            .state_write_within_loop
            .contains_key(&(tid, depth)));
        assert!(!self
            .handler_state
            .writes_in_blocked_loop
            .contains_key(&(tid, depth)));

        self.handler_state
            .reading_addresses
            .insert((tid, depth), Vec::new());
        self.handler_state.comaximal_loops.insert((tid, depth));
        self.handler_state
            .state_write_within_loop
            .insert((tid, depth), FxHashMap::default());
    }

    /// Returns true if the task is blocking, forcing a change point.
    fn mark_spin_loop_end(&mut self, tid: TaskId, cond_flag: &ValuesTypes) -> bool {
        let cond_flag: u32 = {
            if let ValuesTypes::U32(flag) = cond_flag {
                *flag
            } else {
                panic!("Expected cond_flag in spin_loop_end to be U32 but found {cond_flag:?}");
            }
        };
        let cur_depth = *self
            .handler_state
            .loop_depth
            .get(&tid)
            .expect("Must be in a spin_loop before the end");

        let is_comaximal = self
            .handler_state
            .comaximal_loops
            .contains(&(tid, cur_depth));
        debug!("Spin loop ends for {tid:?} depth = {cur_depth}. Cond = {cond_flag} and is_comaximal = {is_comaximal}\n");

        if cond_flag != 0 {
            // sucessfully break the spin_loop
            if self
                .handler_state
                .comaximal_loops
                .contains(&(tid, cur_depth))
            {
                // we are still observing it. Remove from set of watchers.
                self.remove_loop_from_observation(tid, cur_depth);
            }
            // reduce loop depth
            if cur_depth > 1 {
                self.handler_state.loop_depth.insert(tid, cur_depth - 1);
            } else {
                self.handler_state.loop_depth.remove(&tid);
            }
        } else {
            self.handler_state
                .last_action_was_failed_spin_end
                .insert(tid);
            // if it is comaximal, we must wait until we observe a write
            // to one of the addresses. Until then, this task is blocked!
            if self
                .handler_state
                .comaximal_loops
                .contains(&(tid, cur_depth))
            {
                info!("Spin Handler is blocking {tid:?}");
                self.handler_state.blocked_tasks.insert(tid);
                if let Some(map_addr_and_state) = self
                    .handler_state
                    .state_write_within_loop
                    .get(&(tid, cur_depth))
                {
                    let mut list_writes = Vec::new();
                    for (addr, addr_state) in map_addr_and_state {
                        debug!("Status of addr: {addr:?} = {addr_state:?}");
                        if *addr_state == AddressState::Safe {
                            list_writes.push(*addr);
                        }
                    }
                    for write in &list_writes {
                        // as this loop could repeat, we must make sure no
                        // active read is mistaken as co-maximal
                        self.execute_write_event(tid, *write);
                    }
                    trace!("Adding list of writes in blocked_loop: {:?}", list_writes);
                    for write in &list_writes {
                        let cur_cnt = self
                            .handler_state
                            .num_writes_by_blocked_loops
                            .entry(*write)
                            .or_insert(0);
                        *cur_cnt += 1;
                        let cur_list = self
                            .handler_state
                            .blocked_loops_writing_here
                            .entry(*write)
                            .or_default();
                        cur_list.push((tid, cur_depth));
                    }
                    self.handler_state
                        .writes_in_blocked_loop
                        .insert((tid, cur_depth), list_writes);
                }
                return true;
            } else {
                // we are not blocked, and we are already not being observed
                // the loop will repeat itself and all is good!
            }
        }
        false
    }
}

impl ExecuteHandler for SpinLoopHandler {
    fn handle_execute(&mut self, tid: TaskId, ctx_info: &ContextInfo) {
        use lotto::engine::handler::get_value_from_addr;
        use lotto::engine::handler::AddressCatEvent::*;
        use lotto::engine::handler::AddressTwoValuesCatEvent::*;
        use lotto::engine::handler::AddressValueCatEvent::*;
        use lotto::engine::handler::EventArgs::*;

        let mut chpt = false;

        debug!(
            "Executing Spin-loop at action tid = {:?}, args = {:?}",
            tid, ctx_info.event_args
        );
        match &ctx_info.event_args {
            AddressOnly(BeforeRead, addr, _size) => {
                self.execute_read_event(tid, *addr);
            }
            AddressOnly(BeforeARead, addr, _size) => {
                self.execute_read_event(tid, *addr);
            }
            AddressOnly(BeforeWrite, addr, _size) => {
                chpt |= self.execute_write_event(tid, *addr);
            }
            AddressValue(BeforeRMW, addr, size, value) => {
                self.execute_read_event(tid, *addr);
                if get_value_from_addr(addr, size) != *value {
                    // if the value will change, we notify a write as well
                    chpt |= self.execute_write_event(tid, *addr);
                }
            }
            AddressValue(BeforeAWrite, addr, _size, _value) => {
                chpt |= self.execute_write_event(tid, *addr);
            }
            AddressTwoValuesEvent(BeforeCMPXCHG, addr, size, value1, _value2) => {
                if get_value_from_addr(addr, size) != *value1 {
                    // only read
                    self.execute_read_event(tid, *addr);
                } else {
                    // RMW
                    self.execute_read_event(tid, *addr);
                    chpt |= self.execute_write_event(tid, *addr);
                }
            }
            _ => { /* Nothing */ }
        }

        self.handler_state.last_write_changed_blocked_list = chpt;
    }

    fn enabled(&self) -> bool {
        HANDLER.cfg.enabled.load(Ordering::Relaxed)
    }
}

use lotto::engine::handler::EventResult;
impl ExecuteArrivalHandler for SpinLoopHandler {
    fn handle_arrival(
        &mut self,
        tid: TaskId,
        ctx_info: &ContextInfo,
        _active_tids: &Vec<TaskId>,
    ) -> EventResult {
        let mut is_chpt = self.handler_state.last_write_changed_blocked_list;
        is_chpt |= self.handler_state.is_chpt;

        debug!(
            "Executing Spin-loop at arrival tid = {:?}, args = {:?}",
            tid, ctx_info.event_args
        );

        let blocked_tasks = self
            .handler_state
            .blocked_tasks
            .clone()
            .into_iter()
            .collect();
        EventResult::new(blocked_tasks, is_chpt)
    }
}

use std::sync::atomic::AtomicBool;
use std::sync::atomic::Ordering;
use std::sync::LazyLock;

static HANDLER: LazyLock<SpinLoopHandler> = LazyLock::new(SpinLoopHandler::new);

pub fn register() {
    {
        let _ = spin_start_cat();
        let _ = spin_end_cat();
        let spin_start_cat_num = spin_start_cat().0;
        let spin_end_cat_num = spin_end_cat().0;
        let table = &mut lotto::engine::handler::ENGINE_DATA
            .try_lock()
            .expect("single threaded")
            .custom_cat_table;
        table.insert(spin_start_cat_num, parse_spin_start_context());
        table.insert(spin_end_cat_num, parse_spin_end_context());
    }
    lotto::engine::pubsub::subscribe_arrival_or_execute(&*HANDLER);
    lotto::brokers::statemgr::register(&*HANDLER);
}

pub fn register_flags() {
    let _ = FLAG_HANDLER_SPIN_LOOP_ENABLED.get();
}

static FLAG_HANDLER_SPIN_LOOP_ENABLED: FlagKey = FlagKey::new(
    c"FLAG_HANDLER_SPIN_LOOP_ENABLED",
    c"",
    c"handler-spin-loop",
    c"",
    c"enable the spin-loop handler",
    Value::Bool(false),
    &STR_CONVERTER_BOOL,
    Some(_handler_spin_loop_enabled_callback),
);

unsafe extern "C" fn _handler_spin_loop_enabled_callback(
    v: lotto::raw::value,
    _: *mut std::ffi::c_void,
) {
    let enabled = Value::from(v).is_on();
    HANDLER.cfg.enabled.store(enabled, Ordering::Relaxed);
}
