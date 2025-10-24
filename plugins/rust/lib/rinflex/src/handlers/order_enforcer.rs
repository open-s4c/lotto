/// # Order Enforcer
///
/// - Supply ordering constraints through config
/// - During execution, only counters are modified
/// - Upon exit, the results are saved in FINAL states
use handlers::event;
use handlers::event::get_event;
use lotto::base::reason::*;
use lotto::base::TidSet;
use lotto::brokers::statemgr::*;
use lotto::collections::FxHashMap;
use lotto::engine::handler::*;
use lotto::log::*;
use lotto::raw;
use lotto::raw::base_category;
use lotto::Stateful;
use std::sync::LazyLock;

use crate::*;

pub static HANDLER: LazyLock<OrderEnforcer> = LazyLock::new(|| OrderEnforcer {
    cfg: Config::default(),
    fin: Final::default(),
    block: FxHashMap::default(),
    shutdown: false,
});

#[derive(Stateful)]
pub struct OrderEnforcer {
    #[config]
    pub cfg: Config,
    #[result]
    pub fin: Final,

    /// Constraint bookkeeping.
    pub block: FxHashMap<TaskId, u64>,

    /// Handler stopped. Do not respond to `TOPIC_NEXT` anymore.
    pub shutdown: bool,
}

impl Handler for OrderEnforcer {
    fn handle(&mut self, ctx: &raw::context_t, cappt: &mut raw::event_t) {
        if ctx.cat == base_category::CAT_NONE || self.fin.constraints.len() == 0 {
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

        let tset = unsafe { TidSet::wrap(&cappt.tset) };
        for id in tset.iter() {
            if self.block.get(&id).is_some() {
                continue;
            }
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
    }
}

impl ExecuteHandler for OrderEnforcer {
    fn handle_execute(&mut self, tid: TaskId, ctx_info: &ContextInfo) {
        if self.shutdown
            || ctx_info.cat == base_category::CAT_NONE
            || self.fin.constraints.is_empty()
        {
            return;
        }

        /* The sequencer can pick a blocked thread if we returned an empty
         * tset. Guard against this case here (not in the handler to give
         * the execution more opportunity to succeed.)
         */
        if let Some(cnt) = self.block.get(&tid) {
            self.fin.should_discard = true;
            debug!("Shutting down due to Lotto picking thread {} for ANY_TASK which is blocked by {} constraints", tid, cnt);
            return;
        }

        /* Reconstruct last event */
        let last_e = match get_event(tid) {
            Some(e) => e,
            None => panic!(
                "Cannot retrieve an event for task {} (cat {})",
                tid, ctx_info.cat
            ),
        };

        /* Update constraint bookkeeping */
        for constraint in &mut self.fin.constraints {
            let source = &mut constraint.c.source;
            let target = &mut constraint.c.target;

            /* check if *last_event* is a target */
            if target.equals(&last_e) && target.cnt > 0 {
                target.cnt -= 1;
                continue;
            }

            /* check if *last_event* is a source, in that case, (possibly)
             * unblock c, and decrease source counter. */
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
        let already_has = |c: &Constraint| {
            self.fin
                .constraints
                .iter()
                .any(|old| old.c.source.equals(&c.c.source) && old.c.target.equals(&c.c.target))
        };
        let unseen: Vec<_> = self
            .cfg
            .new_constraints
            .iter()
            .filter(|new| !already_has(new))
            .collect();
        debug!("Adding {} new constraints: {:#?}", unseen.len(), unseen);
        self.fin.constraints.extend(unseen.into_iter().cloned());
        // Do not clear new_constraints so that lotto show can display them.
    }
}

/// Check whether event `cur` will break constraint `c`.
///
/// If true, `cur.id` should be blocked.
fn should_block(cur: &Event, constraint: &Constraint) -> bool {
    let source = &constraint.c.source;
    let target = &constraint.c.target;
    debug!(
        "checking [t:{},cat:{},cnt:{}] with {}",
        cur.t.id, cur.t.cat, cur.cnt, constraint
    );

    if source.cnt == 0 || target.cnt != 1 {
        // This constraint will not be broken however the scheduling is.
        return false;
    }

    // At this point, source.cnt != 0 && target.cnt == 1.

    if target.equals(cur) {
        // Apparently, proceeding with target will break the constraint.
        return true;
    }

    if target.t.cat == base_category::CAT_AFTER_CMPXCHG_S
        && cur.t.cat == base_category::CAT_BEFORE_CMPXCHG
        && cur.t.pc == target.t.pc
        && cur.stacktrace == target.stacktrace
        && source.m.is_some()
        && cur.m.is_some()
        && source
            .m
            .clone()
            .unwrap()
            .conflicting_with(&cur.m.clone().unwrap())
        && cur.m.clone().unwrap().predict_cas()
    {
        /* CAS check: effectively consider AFTER_CMPXCHG_S as "BEFORE_CAS
         * that is predicted to succeed".
         *
         * This is because E -> T -> AFTER_CMPXCHG_S is equivalent to E ->
         * AFTER_CMPXCHG_S -> T.
         *
         * Therefore, if target is AFTER_CMPXCHG_S, we need to insert T
         * before the corresponding BEFORE_CMPXCHG, which is only correct
         * when BEFORE_CMPXCHG is followed immediately by AFTER_CMPXCHG_S,
         * so we need to predict whether it will succeed.
         */
        debug!("cas-block-s");
        return true;
    }

    if target.t.cat == base_category::CAT_AFTER_CMPXCHG_F
        && cur.t.cat == base_category::CAT_BEFORE_CMPXCHG
        && cur.t.id == target.t.id
        && cur.t.pc == target.t.pc
        && cur.stacktrace == target.stacktrace
        && source.m.is_some()
        && cur.m.is_some()
        && source
            .m
            .clone()
            .unwrap()
            .conflicting_with(&cur.m.clone().unwrap())
        && !cur.m.clone().unwrap().predict_cas()
    {
        /* Similar to CAT_AFTER_CMPXCHG_S. */
        debug!("cas-block-f");
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
    lotto::engine::pubsub::subscribe_execute(&*HANDLER);
}

pub fn register_flags() {}

//
// Interfaces for CLI
//
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

pub fn get_final() -> ConstraintSet {
    let fin = unsafe { HANDLER.final_mut() };
    fin.constraints.clone()
}

pub fn should_discard() -> bool {
    HANDLER.fin.constraints.len() > 0 && HANDLER.fin.should_discard
}
