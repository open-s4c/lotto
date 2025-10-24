use std::path::{Path, PathBuf};

use lotto::base::category::Category;
use lotto::base::envvar::EnvScope;
use lotto::base::flags::*;
use lotto::base::Trace;
use lotto::base::{Clock, Record, TaskId, Value};
use lotto::cli::flags::*;
use lotto::engine::flags::*;
use lotto::log::*;
use lotto::owned::Owned;
use lotto::raw;
use lotto::sys::now;
use lotto::sys::Stream;

use crate::error::Error;
use crate::handlers;
use crate::inflex::{always, checked_execute, should_discard_execution};
use crate::progress::ProgressBar;
use crate::{inflex, trace, Constraint, ConstraintSet, Event, PrimitiveConstraint};

/// A state machine representing the process of Recursive Inflex.
pub struct RecInflex {
    pub input: PathBuf,
    pub output: PathBuf,
    pub rounds: u64,
    pub report_progress: bool,
    pub use_linear_backoff: bool,

    //
    // Execution parameters
    //
    pub flags: Owned<Flags>,

    //
    // OC states
    //
    pub constraints: ConstraintSet,

    //
    // Internal
    //
    pub tempdir: PathBuf,
    pub trace_fail: PathBuf,
    pub trace_success: PathBuf,
    pub trace_temp: PathBuf,
}

impl RecInflex {
    pub fn new_with_flags(flags: &Flags) -> RecInflex {
        let input = flags.get_path_buf(&FLAG_INPUT);
        let output = flags.get_path_buf(&FLAG_OUTPUT);
        let tempdir = flags.get_path_buf(&FLAG_TEMPORARY_DIRECTORY);
        let trace_fail = tempdir.join("rinflex.fail.trace");
        let trace_success = tempdir.join("rinflex.success.trace");
        let trace_temp = tempdir.join("rinflex.temp.trace");
        let trace = Trace::load_file(&input);
        trace.save(&trace_fail);
        RecInflex {
            input,
            output,
            rounds: flags.get_uval(&FLAG_ROUNDS),
            report_progress: true,
            use_linear_backoff: true,
            flags: flags.to_owned(),
            constraints: Vec::new(),
            tempdir,
            trace_fail,
            trace_success,
            trace_temp,
        }
    }

    /// Check if the loop should terminate.
    pub fn should_terminate(&self) -> Result<bool, Error> {
        let with_oc = self.attach_constraints_to_trace(&self.trace_fail, 0, &self.constraints)?;
        let _replay = EnvScope::new("LOTTO_REPLAY", &with_oc);
        let _record = EnvScope::new("LOTTO_RECORD", &self.trace_temp);
        let _silent = EnvScope::new("LOTTO_LOG_LEVEL", "silent");
        let mut flags = self.flags.clone();
        flags.set_by_opt(&FLAG_REPLAY_GOAL, Value::U64(0));
        let mut bar = ProgressBar::new(self.report_progress, "HALT", self.rounds);
        crate::inflex::always(self.rounds, || {
            let exitcode = loop {
                flags.set_by_opt(&flag_seed(), Value::U64(now()));
                let exitcode = checked_execute(&with_oc, &flags, true)?;
                if should_discard_execution(&self.trace_temp)? {
                    bar.tick_invalid();
                    continue;
                }
                break exitcode;
            };
            bar.tick_valid();
            Ok(exitcode != 0)
        })
    }

    /// Do one iteration of the Recursive Inflex main loop.
    pub fn find_next_pair(&mut self) -> Result<PrimitiveConstraint, Error> {
        let mut symm_set = Vec::new();
        self.inflex_pair(0, 0, &mut symm_set)
    }

    /// Do one iteration of the Recursive Inflex main loop.
    fn inflex_pair(
        &mut self,
        inflex_min: Clock,
        depth: usize,
        symm_set: &mut Vec<PrimitiveConstraint>,
    ) -> Result<PrimitiveConstraint, Error> {
        // find IP
        info!(
            "Find next inflex pair, depth={}, inflex_min={}",
            depth, inflex_min
        );
        info!("Finding IP...");
        let mut inflex = inflex::Inflex::new_with_flags_and_input(&self.flags, &self.trace_fail);
        inflex.output = self.trace_temp.clone();
        inflex.min = inflex_min;
        inflex.report_progress = self.report_progress;
        let ip = inflex.run_fast()?;
        info!("IP is {}", ip);
        if ip == 0 {
            panic!("IP shouldn't be 0");
        }
        let source = self.event_at_clock(&self.flags, &self.trace_fail, ip)?;
        info!("Source event is\n{}", source.display()?);

        // Find a successful run by replaying trace_fail up to IP-1
        info!("Finding a successful trace up to IP-1...");
        self.get_trace(
            Outcome::Success,
            ip.checked_sub(1).expect("IP cannot be 0"),
            &self.trace_fail,
            &self.trace_success,
        )?;

        // find IIP
        info!("Finding IIP...");
        let mut inflex = inflex::Inflex::new_with_flags_and_input(&self.flags, &self.trace_success);
        inflex.output = self.trace_temp.clone();
        inflex.report_progress = self.report_progress;
        inflex.min = ip - 1;
        let iip = inflex.run_inverse()?;
        info!("IIP is {}", iip);
        let target = self.event_at_clock(&self.flags, &self.trace_success, iip)?;
        info!("Target event is\n{}", target.display()?);

        // This is the IP-IIP pair.
        let pair = PrimitiveConstraint { source, target };

        // Quick path: IIP must be IP's sibling, if IIP = IP and they
        // are derived from the same prefix.
        // if iip == ip {
        //     info!("Correct (quick path)");
        //     return Ok(pair);
        // }

        // Check for symmetric ordering constraints.
        //
        // It is possible that following a prefix, there are IP, IIP1,
        // and IIP2. Both IP->IIP1 and IP->IIP2 should be included.
        //
        // But in the following Multiple Bug Handling algorithm, it
        // will be incorrectly labeled as incorrect and dropped.
        // Here, check if the OC is repeatedly found. If it is
        // repeated, it means the OC is actually correct.
        for s in symm_set.iter() {
            if s.equals(&pair) {
                info!("Repeated OC considered correct");
                return Ok(pair);
            }
        }

        // multiple bug handling.
        if self.pair_can_reproduce_bug(&pair, ip)? {
            info!("Correct");
            Ok(pair)
        } else {
            info!("Incorrect");
            self.get_trace(
                Outcome::Fail,
                iip - 1,
                &self.trace_success,
                &self.trace_fail,
            )?;
            symm_set.push(pair);
            self.inflex_pair(iip - 1, depth + 1, symm_set)
        }
    }

    /// Replaying on the failing trace up to ip-1 while enforcing
    /// pair, checks whether it will fail.
    fn pair_can_reproduce_bug(
        &mut self,
        pair: &PrimitiveConstraint,
        ip: u64,
    ) -> Result<bool, Error> {
        self.push(Constraint {
            virt: true,
            positive: true,
            c: pair.clone(),
        });
        let check_trace =
            self.attach_constraints_to_trace(&self.trace_fail, ip - 1, &self.constraints);
        self.pop();
        info!("Checking if the pair will lead to the bug when enforced");
        let check_trace = check_trace?;
        let mut flags = self.flags.to_owned();
        let _replay = EnvScope::new("LOTTO_REPLAY", &check_trace);
        let _record = EnvScope::new("LOTTO_RECORD", &self.trace_temp);
        let _silent = EnvScope::new("LOTTO_LOG_LEVEL", "silent");
        let mut bar = ProgressBar::new(self.report_progress, "correct?", self.rounds);
        always(self.rounds, || {
            let mut invalid_cnt = 0;
            let exitcode = loop {
                // We tried for 100r times, yet no single valid run found.
                // Use a separate invalid_cnt because bar.invalid_ticks is shared.
                if bar.valid_ticks == 0 && invalid_cnt > 100 * self.rounds {
                    warn!("Cannot find a valid execution");
                    return Err(Error::ExecutionNotFound);
                }
                flags.set_by_opt(&flag_seed(), Value::U64(now()));
                let exitcode = checked_execute(&check_trace, &flags, true)?;
                if should_discard_execution(&self.trace_temp)? {
                    bar.tick_invalid();
                    invalid_cnt += 1;
                    continue;
                }
                break exitcode;
            };
            bar.tick_valid();
            Ok(exitcode != 0)
        })
    }

    /// Check if `pair` is essential to the reproduction of the bug.
    ///
    /// It's essential if all runs (from clock 0) dissatisfying `pair`
    /// are successful.
    pub fn check_if_essential(&mut self, pair: &PrimitiveConstraint) -> Result<bool, Error> {
        let oc = Constraint {
            c: pair.flipped(), /* dissatisfy pair */
            virt: true,
            positive: false,
        };
        self.push(oc);
        let out = self.attach_constraints_to_trace(&self.input, 0, &self.constraints);
        self.pop();
        let out = out?;
        let _replay = EnvScope::new("LOTTO_REPLAY", &out);
        let _record = EnvScope::new("LOTTO_RECORD", &self.trace_temp);
        let _silent = EnvScope::new("LOTTO_LOG_LEVEL", "silent");
        let mut flags = self.flags.to_owned();
        let mut bar = ProgressBar::new(self.report_progress, "", self.rounds);
        let always_succeed = always(self.rounds, || {
            let exitcode = loop {
                flags.set_by_opt(&flag_seed(), Value::U64(now()));
                let exitcode = checked_execute(&out, &flags, true)?;
                if should_discard_execution(&self.trace_temp)? {
                    bar.tick_invalid();
                    continue;
                }
                break exitcode;
            };
            bar.tick_valid();
            Ok(exitcode == 0)
        })?;
        bar.done();
        Ok(always_succeed)
    }

    /// Similar to [`get_trace`], but it tries to search backwards
    /// from `upper_bound`.
    ///
    /// This is under an assumption that the more you replay, the more
    /// likely you hit the bug.
    pub fn get_trace_from_zero(
        &self,
        outcome: Outcome,
        input: &Path,
        output: &Path,
    ) -> Result<(), Error> {
        let (input, last_valid_clk) =
            self.set_constraints_in_first_config_record(input, &self.constraints)?;
        let _replay = EnvScope::new("LOTTO_REPLAY", &input);
        let _record = EnvScope::new("LOTTO_RECORD", &output);
        let _env_silent = EnvScope::new("LOTTO_LOG_LEVEL", "silent");
        let mut flags = self.flags.clone();
        let mut cnt = 0;
        let mut replay_goal = last_valid_clk;
        let mut backoff = 1u64;
        let mut bar = ProgressBar::new(self.report_progress, "", self.rounds);
        bar.set_prefix_string(format!("goal={}", replay_goal));
        loop {
            cnt += 1;
            if cnt > self.rounds {
                if replay_goal == 0 {
                    warn!("Cannot find satisfying execution at clock 0 in get_trace_from_zero");
                    return Err(Error::ExecutionNotFound);
                }
                cnt = 0;
                if self.use_linear_backoff {
                    backoff = (last_valid_clk / 20).max(1);
                } else {
                    backoff = (backoff * 3 + 1) / 2;
                }
                replay_goal = replay_goal.saturating_sub(backoff);
                bar.set_prefix_string(format!("goal={}", replay_goal));
                bar.reset();
                continue;
            }
            flags.set_by_opt(&FLAG_REPLAY_GOAL, Value::U64(replay_goal));
            flags.set_by_opt(&flag_seed(), Value::U64(now()));
            let exitcode = checked_execute(&input, &flags, true)?;
            if should_discard_execution(&output)? {
                bar.tick_invalid();
                continue;
            }
            bar.tick_valid();
            if outcome.matches(exitcode) {
                return Ok(());
            }
        }
    }

    pub fn get_trace(
        &self,
        outcome: Outcome,
        replay_goal: Clock,
        input: &Path,
        output: &Path,
    ) -> Result<(), Error> {
        let input = self.attach_constraints_to_trace(input, replay_goal, &self.constraints)?;
        let _replay = EnvScope::new("LOTTO_REPLAY", &input);
        let _record = EnvScope::new("LOTTO_RECORD", &output);
        let _env_silent = EnvScope::new("LOTTO_LOG_LEVEL", "silent");
        let mut flags = self.flags.clone();
        let mut bar = ProgressBar::new(self.report_progress, "", self.rounds * 10);
        loop {
            if bar.valid_ticks > self.rounds * 10
                || bar.valid_ticks == 0 && bar.invalid_ticks > self.rounds * 10
            {
                warn!("Cannot find satisfying execution in get_trace");
                return Err(Error::ExecutionNotFound);
            }
            flags.set_by_opt(&FLAG_REPLAY_GOAL, Value::U64(replay_goal));
            flags.set_by_opt(&flag_seed(), Value::U64(now()));
            let exitcode = checked_execute(&input, &flags, true)?;
            if should_discard_execution(&output)? {
                bar.tick_invalid();
                continue;
            }
            bar.tick_valid();
            if outcome.matches(exitcode) {
                return Ok(());
            }
        }
    }

    fn event_at_clock(&self, flags: &Flags, trace: &Path, clock: Clock) -> Result<Event, Error> {
        let _silent = EnvScope::new("LOTTO_LOG_LEVEL", "silent");
        let event = trace::get_event_at_clk(flags, trace, clock)?;

        // Not CAS.
        if event.t.cat != Category::CAT_BEFORE_CMPXCHG {
            return Ok(event);
        }

        // For CAS operations, Inflex and InverseInflex will always
        // find BEFORE_CMPXCHG, which is always followed by, and only
        // followed by AFTER_CMPXCHG_S or AFTER_CMPXCHG_F.
        //
        // (Justification: BEFORE_CMPXCHG is always followed by
        // AFTER_CMPXCHG_[SF], and whenever AFTER_CMPXCHG_[SF] is
        // causing a bug / invalidating a bug, BEFORE_CMPXCHG will be
        // selected as IP/IIP.)
        //
        // The pattern will always be like:
        //   ... B F ... B F ... B S
        // where ... denotes other events, possibly happening inside
        // the CAS loop body.
        //
        // The PC counts are obviously wrong: each B and F's counts
        // are incrementing, but they should be considered the same
        // atomic operation. Here, replace BEFORE_CMPXCHG as
        // AFTER_CMPXCHG_S because that is the true, intended meaning
        // of the program.
        //
        // The handling is correct for CAS loops: they always
        // terminate with a SUCCESS.
        //
        // However, there is another pattern:
        //   if (atomic_compare_exchange(...)) {
        //       /* do something */
        //   } else {
        //       /* do something else */
        //   }
        //
        // In this case, B, S, and F's counts are correct. We simply
        // keep BEFORE_CMPXCHG to expose more events. Unfortunately,
        // it is impossible to tell this pattern from a one-shot
        // successful CAS loop. This is probably problematic.
        let after_event = trace::get_event_at_clk(flags, trace, clock + 1)?;
        if after_event.t.cat == Category::CAT_AFTER_CMPXCHG_S {
            return Ok(after_event);
        } else {
            return Ok(event);
        }
    }

    /// Returns the output trace file, as well as the last clock of
    /// the output trace. The clock corresponds to the last event
    /// which is not affected by the constraints.
    pub fn set_constraints_in_first_config_record(
        &self,
        input: &Path,
        constraints: &ConstraintSet,
    ) -> Result<(PathBuf, Clock), Error> {
        if constraints.len() == 0 {
            let last_clk = trace::get_last_clk(input);
            return Ok((input.to_owned(), last_clk));
        }

        let lowest_clk = self
            .constraints
            .iter()
            .fold(u64::MAX, |res, r| res.min(r.c.clock_lower_bound()))
            .saturating_sub(1); // Account for `event_at_clock` which may return the *next* event

        let input_filename = input.file_name().expect("must be a file").to_str().unwrap();
        let output_path = self.tempdir.join(format!("{}+oc", input_filename));

        let mut input = Trace::load_file(input);
        let mut st = Stream::new_file_out(&output_path);
        let mut output = Trace::new_file(&mut st);
        let mut last_clk = 0;

        while let Some(r) = input.next_any() {
            // Only keep records in the range [0 .. start_clk - 1].
            // All records from start_clk are potentially invalidated
            // by the constraints.
            if r.clk >= lowest_clk {
                break;
            }
            last_clk = r.clk;
            if r.kind == raw::record::RECORD_CONFIG && r.clk == 0 {
                r.unmarshal();
                handlers::order_enforcer::cli_set_constraints(constraints.clone());
                let r = Record::new_config(r.clk);
                output.append(r).expect("append updated config");
                handlers::order_enforcer::cli_clear_constraints();
            } else {
                output.append(r.to_owned()).expect("append old record");
            }
            input.advance();
        }

        // Make sure we can replay up to lowest_clk - 1.
        if last_clk != lowest_clk - 1 {
            let mut r = Record::new(0);
            r.clk = lowest_clk - 1;
            r.kind = raw::record::RECORD_OPAQUE;
            output.append(r).expect("OPAQUE");
        }

        Ok((output_path, lowest_clk - 1))
    }

    /// Attach constraints to trace through order_enforcer's config.
    ///
    /// If goal is non-zero, it will update the constraints to the
    /// specified clock automatically by using `update_constraints`.
    pub fn attach_constraints_to_trace(
        &self,
        input: &Path,
        goal: Clock,
        constraints: &ConstraintSet,
    ) -> Result<PathBuf, Error> {
        if constraints.len() == 0 {
            return Ok(input.to_owned());
        }

        let input_filename = input.file_name().expect("must be a file").to_str().unwrap();
        let out = self.tempdir.join(format!("{}+oc", input_filename));

        let all = if goal == 0 {
            constraints.clone()
        } else {
            self.update_constraints(input, goal, constraints)?
        };
        handlers::order_enforcer::cli_set_constraints(all);

        let mut rec = Trace::load_file(input);
        rec.trim_to_goal(goal, true);
        let r = Record::new_config(goal);
        rec.append(r).expect("append updated oc to trace");
        rec.save(&out);
        handlers::order_enforcer::cli_clear_constraints();

        Ok(out)
    }

    pub fn update_constraints(
        &self,
        input: &Path,
        goal: Clock,
        constraints: &ConstraintSet,
    ) -> Result<ConstraintSet, Error> {
        trace::expand(&self.flags, input, &self.trace_temp, Some(goal));
        let mut constraints = constraints.clone();
        let mut rec = Trace::load_file(&self.trace_temp);
        while let Some(r) = {
            rec.advance();
            rec.next_any_mut()
        } {
            if r.clk > goal {
                break;
            }
            r.unmarshal();
            let Some(event) = handlers::event::get_event(TaskId(r.id)) else {
                continue;
            };
            let transition = event.t;
            let stacktrace = event.stacktrace;
            for constraint in &mut constraints {
                if constraint.c.target.t == transition
                    && constraint.c.target.stacktrace == stacktrace
                {
                    constraint.c.target.cnt = constraint.c.target.cnt.saturating_sub(1);
                    if constraint.c.target.cnt == 0 && constraint.c.source.cnt != 0 {
                        panic!(
                            "Source must have been satisfied, source={}, target={}",
                            constraint.c.source, constraint.c.target
                        );
                    }
                }
                if constraint.c.source.t == transition
                    && constraint.c.source.stacktrace == stacktrace
                {
                    constraint.c.source.cnt = constraint.c.source.cnt.saturating_sub(1);
                }
            }
        }
        Ok(constraints)
    }

    #[allow(dead_code)]
    pub fn has_constraint(&self, rhs: &Constraint) -> bool {
        for c in self.constraints.iter() {
            if c.c.equals(&rhs.c) {
                return true;
            }
        }
        false
    }

    #[inline(always)]
    fn push(&mut self, c: Constraint) {
        self.constraints.push(c)
    }

    #[inline(always)]
    fn pop(&mut self) {
        let _c = self.constraints.pop().expect("pop must match a push");
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Outcome {
    Success,
    Fail,
}

impl Outcome {
    fn matches(self, code: i32) -> bool {
        match self {
            Outcome::Success => code == 0,
            Outcome::Fail => code != 0,
        }
    }
}

#[allow(dead_code)]
/// For debugging.
fn pause(prompt: &str) {
    use std::io::Write;
    print!("{}", prompt);
    std::io::stdout().flush().unwrap();
    let mut s = String::new();
    std::io::stdin().read_line(&mut s).unwrap();
}
