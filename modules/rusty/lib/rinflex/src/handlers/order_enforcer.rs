//! # Order Enforcer
//!
//! - Supply ordering constraints through config
//! - During execution, only counters are modified
//! - Upon exit, the results are saved in FINAL states
use lotto::base::reason::*;
use lotto::base::Category;
use lotto::base::TidSet;
use lotto::base::Value;
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
    prev_any_task_filter: None,
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
pub struct OrderEnforcer {
    #[config]
    pub cfg: Config,
    #[result]
    pub fin: Final,

    /// Constraint bookkeeping.
    pub block: BTreeMap<TaskId, u64>,

    /// Handler stopped. Do not respond to `TOPIC_NEXT` anymore.
    pub shutdown: bool,

    /// The maximal clock that this execution is allowed to reach.
    pub max_clock: U64OrInf,

    prev_any_task_filter: Option<unsafe extern "C" fn(raw::task_id) -> bool>,
}

impl Handler for OrderEnforcer {
    fn handle(&mut self, ctx: &raw::context_t, cappt: &mut raw::event_t) {
        if ctx.cat == Category::CAT_NONE || self.fin.constraints.len() == 0 {
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

        if U64OrInf::from(cappt.clk) > self.max_clock {
            self.shutdown = true;
            cappt.reason = REASON_IGNORE;
            return;
        }

        // Check if we should block the current thread.
        let constraints = &mut self.fin.constraints;

        if constraints.len() == 0 {
            return;
        }

        let tset = unsafe { TidSet::wrap(&cappt.tset) };
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
                }
            }
        }

        for block_id in self.block.keys() {
            unsafe {
                raw::tidset_remove(&mut cappt.tset, block_id.0);
            }
        }

        /* It is possible that tset is empty at this point.  In that case,
         * the engine can pick any task, including ones that are already
         * blocked, which is handled in TOPIC_NEXT_TASK above. */

        /* Reduce the likelihood by setting any_task_filter. */
        if self.block.len() != 0 {
            self.prev_any_task_filter = cappt.any_task_filter;
            cappt.any_task_filter = Some(_any_task_filter_blocked_by_constraints);
        }
    }

    fn posthandle(&mut self, ctx: &raw::context_t) {
        if self.shutdown || ctx.cat == Category::CAT_NONE || self.fin.constraints.is_empty() {
            return;
        }

        let tid = TaskId(ctx.id);

        /* The sequencer can pick a blocked thread if we returned an empty
         * tset. Guard against this case here (not in the handler to give
         * the execution more opportunity to succeed.)
         */
        if let Some(cnt) = self.block.get(&tid) {
            self.fin.should_discard = true;
            debug!("Shutting down due to Lotto picking thread {} for ANY_TASK which is blocked by {} constraints", tid, cnt);
            debug!(
                "discard from order_enforcer because task {} is blocked",
                tid
            );
            return;
        }

        /* Reconstruct last event */
        let last_e = match event::get_event(tid) {
            Some(e) => e,
            None => panic!(
                "Cannot retrieve an event for task {} (cat {})",
                tid, ctx.cat
            ),
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
fn should_block(cur: &Event, constraint: &Constraint) -> bool {
    let source = &constraint.c.source;
    let target = &constraint.c.target;
    if cur.t == source.t || cur.t == target.t {
        debug!(
            "checking [t:{},cat:{},cnt:{},eff:{:?}] against {}",
            cur.t.id,
            cur.t.cat,
            cur.cnt,
            Eff::from(cur.m.as_ref()),
            constraint
        );
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

#[derive(Encode, Decode, Debug, Default)]
pub struct Config {
    pub new_constraints: ConstraintSet,
}

impl Marshable for Config {
    fn print(&self) {
        info!("{} new constraints", self.new_constraints.len());
        for (i, c) in self.new_constraints.iter().enumerate() {
            info!("New constraint {} = {}", i, c);
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
    trace!("Registering rinflex handler");
    lotto::engine::handler::register(&*HANDLER);
    lotto::brokers::statemgr::register(&*HANDLER);
    lotto::brokers::statemgr::subscribe_to_statemgr_topics(&*HANDLER);
}

// ANY_TASK filter

unsafe extern "C" fn _any_task_filter_blocked_by_constraints(task_id: u64) -> bool {
    if let Some(f) = HANDLER.prev_any_task_filter {
        if !f(task_id) {
            return false;
        }
    }
    HANDLER.block.get(&TaskId(task_id)).is_none()
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
