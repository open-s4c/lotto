//! # Order Enforcer
//!
//! - Supply ordering constraints through config
//! - During execution, only counters are modified
//! - Upon exit, the results are saved in FINAL states
#![allow(unused_imports)]

use lotto::base::reason::*;
use lotto::base::{CapturePoint, TidSet, Value};
use lotto::brokers::statemgr::*;
use lotto::cli::flags::STR_CONVERTER_NONE;
use lotto::cli::FlagKey;
use lotto::engine::handler::*;
use lotto::log::*;
use lotto::raw;
use lotto::sync::HandlerWrapper;
use lotto::Stateful;

use std::collections::BTreeMap;

use crate::handlers::event;
use crate::num::U64OrInf;
use crate::*;

pub static HANDLER: HandlerWrapper<OrderEnforcer> = HandlerWrapper::new(|| OrderEnforcer {
    cfg: Config::default(),
    fin: Final::default(),
    block: BTreeMap::new(),
    shutdown: false,
    max_clock: get_max_clock(),
    prev_any_task_filter: BTreeMap::new(),
});

fn get_max_clock() -> U64OrInf {
    let max_clock_str = std::env::var("LOTTO_RINFLEX_MAX_CLOCK").unwrap_or_default();
    let max_clock_uval = max_clock_str.parse::<u64>().unwrap_or_default();
    if max_clock_uval <= 1 {
        U64OrInf::inf()
    } else {
        U64OrInf::from(max_clock_uval)
    }
}

#[derive(Stateful)]
#[allow(dead_code)]
pub struct OrderEnforcer {
    #[config]
    pub cfg: Config,
    #[result]
    pub fin: Final,

    /// Constraint bookkeeping.
    pub block: BTreeMap<TaskId, u64>,

    /// Handler stopped. Do not respond to `EVENT_NEXT` anymore.
    pub shutdown: bool,

    /// The maximal clock that this execution is allowed to reach.
    pub max_clock: U64OrInf,

    prev_any_task_filter: BTreeMap<raw::task_id, Option<unsafe extern "C" fn(raw::task_id) -> bool>>,
}

#[cfg(feature = "runtime")]
macro_rules! discard {
    ($self:expr) => {
        $self.discard();
        return;
    };
}

#[cfg(feature = "runtime")]
impl Handler for OrderEnforcer {
    fn handle(&mut self, ctx: &CapturePoint, cappt: &mut raw::event_t) {
        if U64OrInf::from(cappt.clk) > self.max_clock {
            self.shutdown = true;
            cappt.reason = REASON_IGNORE;
            return;
        }

        if ctx.type_id == 0 || self.fin.constraints.len() == 0 {
            return;
        }

        // Follow decisions made by handler_termination and handler_drop
        if cappt.reason.is_shutdown() {
            self.shutdown = true;
            return;
        }

        // The engine picked a blocked task, or made a wrong CAS
        // prediction.
        if self.fin.should_discard {
            cappt.reason = REASON_IGNORE;
            self.shutdown = true;
            return;
        }

        // Check if we should block the current thread.
        let constraints = &mut self.fin.constraints;
        if constraints.len() == 0 {
            return;
        }
        // If the current task is going to exit, any pending
        // constraint whose source is this task cannot be satisfied
        // anymore. Discard this run immediately.
        if ctx.type_id == raw::EVENT_TASK_FINI as u16 {
            for c in constraints.iter() {
                if c.c.source.cnt > 0 && c.c.source.t.id == TaskId(ctx.id) {
                    debug!("Shutting down due to constraint {} cannot be satisfied because its source is no longer available", c);
                    discard!(self);
                }
            }
        }

        let tset = unsafe { TidSet::wrap_mut(&mut cappt.tset) };
        let mut need_yield = false;
        for id in tset.iter() {
            if self.block.get(&id).is_some() {
                continue;
            }
            // It is possible that a Read event now reads a different
            // value, because another Write event happened before its
            // resumption. Update the read value to the current value.
            event::update_event(id);
            let e = match event::get_rt_event(id) {
                None => {
                    panic!(
                        "Cannot retrieve event for id {}; did you enable handler_event?",
                        id
                    );
                }
                Some(e) => e,
            };
            for c in constraints.iter() {
                if should_block(&e, c) {
                    debug!("Blocking task {} due to constraint {}", e.t.id, c);
                    self.block
                        .entry(id)
                        .and_modify(|cnt| *cnt += 1)
                        .or_insert(1);
                } else if id.0 == ctx.id && should_yield(&e, c) {
                    need_yield = true;
                }
            }
        }

        for block_id in self.block.keys() {
            tset.remove(TaskId(block_id.0));
        }
        if need_yield && tset.size() >= 2 {
            tset.remove(TaskId(ctx.id));
        }

        /* It is possible that tset is empty at this point.  In that case,
         * the engine can pick any task, including ones that are already
         * blocked. This will be checked by posthandle. */

        /* Reduce the likelihood by setting any_task_filter. */
        if self.block.get(&TaskId(ctx.id)).is_some() {
            if cappt.any_task_filter.is_some() {
                self.prev_any_task_filter.insert(ctx.id, cappt.any_task_filter);
            } else {
                self.prev_any_task_filter.remove(&ctx.id);
            }
            cappt.any_task_filter = Some(should_wait);
        }
    }

    fn posthandle(&mut self, ctx: &CapturePoint) {
        if self.shutdown || ctx.type_id == 0 || self.fin.constraints.is_empty() {
            return;
        }

        let tid = TaskId(ctx.id);

        /* The sequencer can pick a blocked thread if we returned an empty
         * tset. Guard against this case here (not in the handler to give
         * the execution more opportunity to succeed.)
         */
        if let Some(cnt) = self.block.get(&tid) {
            debug!("Shutting down due to Lotto picking thread {} for ANY_TASK which is blocked by {} constraints", tid, cnt);
            debug!(
                "discard from order_enforcer because task {} is blocked",
                tid
            );
            discard!(self);
        }

        /* Reconstruct last event */
        let Some(last_e) = event::get_event(tid) else {
            return;
        };

        /* Update constraint bookkeeping */
        for constraint in &mut self.fin.constraints {
            let source = &mut constraint.c.source;
            let target = &mut constraint.c.target;

            /* check if last executed instruction is target */
            if target.equals(&last_e) && target.cnt > 0 {
                target.cnt -= 1;
                continue;
            }

            /* check if last executed instruction is source, in that
             * case, (possibly) unblock c, and decrease source
             * counter.
             */
            if source.equals(&last_e) && source.cnt > 0 {
                source.cnt -= 1;
                if source.cnt > 0 {
                    /* We can only unblock target if source is fully
                     * satisfied. */
                    continue;
                }
                let bcnt = self.block.get_mut(&target.t.id);
                if bcnt.is_none() {
                    continue;
                }
                let bcnt = bcnt.unwrap();
                assert!(*bcnt > 0);
                *bcnt -= 1;
                if *bcnt == 0 {
                    debug!("Unblocking {}", target.t.id);
                    self.block.remove(&target.t.id);
                }
            }
        }
    }
}

impl OrderEnforcer {
    #[cfg(feature = "runtime")]
    fn discard(&mut self) {
        self.fin.should_discard = true;
        self.shutdown = true;
    }
}

impl StateTopicSubscriber for OrderEnforcer {
    fn after_unmarshal_config(&mut self) {
        if self.cfg.new_constraints.len() == 0 {
            return;
        }

        // See also [`cli_clear_constraints`]. Lotto will append a
        // config record at replay_goal, which can cause duplicate
        // constraints. Here, only add those unseen before.
        let already_has = |c: &Constraint| self.fin.constraints.iter().any(|old| old.id == c.id);
        let mut unseen: Vec<_> = self
            .cfg
            .new_constraints
            .iter()
            .filter(|new| !already_has(new))
            .cloned()
            .collect();

        // Update event counters
        for c in &mut unseen {
            let e_src = &mut c.c.source;
            let e_tgt = &mut c.c.target;
            let ecore_src = GenericEventCoreRef::from(&*e_src);
            let ecore_tgt = GenericEventCoreRef::from(&*e_tgt);
            let src_cnt = event::get_event_happened_cnt(ecore_src);
            let tgt_cnt = event::get_event_happened_cnt(ecore_tgt);
            e_src.cnt = e_src
                .cnt
                .checked_sub(src_cnt)
                .expect("underflow in constraint's source");
            e_tgt.cnt = e_tgt
                .cnt
                .checked_sub(tgt_cnt)
                .expect("underflow in constraint's target");
        }
        for (i, c) in unseen.iter().enumerate() {
            debug!("add updated constraint {} => {}", i, c);
        }
        self.fin.constraints.extend(unseen);

        // Do not clear new_constraints so that lotto show can display them.
    }
}

/// Check whether event `cur` will break constraint `c`.
///
/// If true, `cur.id` should be blocked.
#[cfg(feature = "runtime")]
fn should_block(cur: &Event, constraint: &Constraint) -> bool {
    let source = &constraint.c.source;
    let target = &constraint.c.target;
    if cur.t == target.t {
        debug!(
            "checking [t:{},cat:{},cnt:{},eff:{:?}] against constraint id={}",
            cur.t.id,
            cur.t.event_name(),
            cur.cnt,
            Eff::from(cur.m.as_ref()),
            constraint.id
        );
    } else {
        return false;
    }

    if source.cnt == 0 || target.cnt != 1 {
        // This constraint will not be broken however the scheduling is.
        return false;
    }

    // At this point, source.cnt != 0 && target.cnt == 1.

    if target.equals(cur) {
        // Apparently, proceeding with target will break the constraint.
        return true;
    }

    debug!("fallback-unblockable");
    false
}

/// If there is (a -> b) and we see a before b, we can force yield to
/// another thread.
///
/// When IP is incorrect (smaller than the true IP), we want to find a
/// counterexample instead of slipping through the true IP.
#[cfg(feature = "runtime")]
fn should_yield(e: &Event, c: &Constraint) -> bool {
    e.t == c.c.source.t && c.c.target.cnt == 1
}

#[derive(Encode, Decode, Debug, Default)]
pub struct Config {
    pub new_constraints: ConstraintSet,
}

impl Marshable for Config {
    fn print(&self) {
        info!("{} new constraints", self.new_constraints.len());
        for (i, c) in self.new_constraints.iter().enumerate() {
            info!(
                "new constraint {} (id={}) [t:{}, clk:{}, pc:{}, cat:{}] => [t:{}, clk:{}, pc:{}, cat:{}]",
                i, c.id,
                c.c.source.t.id,
                c.c.source.clk,
                c.c.source.t.pc,
                c.c.source.t.type_id.name(),
                c.c.target.t.id,
                c.c.target.clk,
                c.c.target.t.pc,
                c.c.target.t.type_id.name(),
            );
        }
    }
}

#[derive(Encode, Decode, Debug, Default)]
pub struct Final {
    pub constraints: ConstraintSet,

    /// Should discard this run because the engine picked a blocked thread for ANY_TASK.
    pub should_discard: bool,
}

impl Marshable for Final {
    fn print(&self) {
        info!("total number of constraints = {}", self.constraints.len());
        for (i, c) in self.constraints.iter().enumerate() {
            info!("Constraint {} = {}", i, c);
        }
        info!("should_discard = {}", self.should_discard);
    }
}

pub fn register() {
    #[cfg(feature = "runtime")]
    {
        trace!("Registering rinflex handler");
        lotto::engine::handler::register(&*HANDLER);
    }
    lotto::brokers::statemgr::register(&*HANDLER);
    lotto::brokers::statemgr::subscribe_to_statemgr_topics(&*HANDLER);
}

// ANY_TASK filter

#[cfg(feature = "runtime")]
unsafe extern "C" fn should_wait(task_id: raw::task_id) -> bool {
    // hard blocked by other modules
    if HANDLER.prev_any_task_filter.get(&task_id)
        .and_then(|f| *f)
        .is_some_and(|f| f(task_id))
    {
        return true;
    }

    // oc-blocked
    let oc_blocked = HANDLER.block.get(&TaskId(task_id)).is_some();
    if !oc_blocked {
        return false;
    }

    // allow id if hard-blocking id will deadlock in switcher
    let aosb = unsafe { TidSet::wrap(raw::available_or_soft_blocked_tasks()) };
    let mut aosb = aosb.clone();
    for id in HANDLER.block.keys() {
        aosb.remove(*id);
    }
    let will_deadlock = aosb.size() == 0;
    !will_deadlock
}

//
// CLI flags
//

unsafe extern "C" fn _flag_rinflex_max_clock_callback(v: raw::value, _: *mut std::ffi::c_void) {
    let v = Value::from(v);
    let max_clock_uval = v.as_uval();
    let max_clock_str = format!("{}", max_clock_uval);
    std::env::set_var("LOTTO_RINFLEX_MAX_CLOCK", max_clock_str);
}

pub static FLAG_RINFLEX_MAX_CLOCK: FlagKey = FlagKey::new(
    c"FLAG_RINFLEX_MAX_CLOCK",
    c"",
    c"rinflex-max-clock",
    c"INT",
    c"Workaround for livelocks",
    Value::U64(0),
    &STR_CONVERTER_NONE,
    Some(_flag_rinflex_max_clock_callback),
);

pub fn register_flags() {
    let _ = FLAG_RINFLEX_MAX_CLOCK.get();
}

//
// Interfaces for CLI
//
pub fn cli_cfg_constraints() -> &'static mut ConstraintSet {
    let cfg = unsafe { HANDLER.config_mut() };
    &mut cfg.new_constraints
}

pub fn cli_set_constraints(constraints: ConstraintSet) {
    let cfg = unsafe { HANDLER.config_mut() };
    let fin = unsafe { HANDLER.final_mut() };
    cfg.new_constraints = constraints;
    fin.constraints.clear();
    fin.should_discard = false;
}

pub fn cli_clear_constraints() {
    let cfg = unsafe { HANDLER.config_mut() };
    cfg.new_constraints.clear();
}

pub fn cli_reset() {
    let fin = unsafe { HANDLER.final_mut() };
    fin.should_discard = false;
    fin.constraints.clear();
}

pub fn should_discard() -> bool {
    HANDLER.fin.constraints.len() > 0 && HANDLER.fin.should_discard
}
