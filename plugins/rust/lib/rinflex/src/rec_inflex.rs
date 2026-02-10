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
use crate::inflex::{always, checked_execute, postexec, Outcome};
use crate::progress::ProgressBar;
use crate::{inflex, trace, Constraint, ConstraintSet, Event, PrimitiveConstraint};

/// A state machine representing the process of Recursive Inflex.
pub struct RecInflex {
    pub input: PathBuf,
    pub output: PathBuf,
    pub rounds: u64,
    pub report_progress: bool,
    pub use_linear_backoff: bool,
    pub next_id: usize,

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
    pub trace_fail_alt: PathBuf,
    pub trace_success: PathBuf,
    pub trace_temp: PathBuf,
}

impl RecInflex {
    pub fn new_with_flags(flags: &Flags) -> RecInflex {
        let input = flags.get_path_buf(&FLAG_INPUT);
        let output = flags.get_path_buf(&FLAG_OUTPUT);
        let tempdir = flags.get_path_buf(&FLAG_TEMPORARY_DIRECTORY);
        let trace_fail = tempdir.join("rinflex.fail.trace");
        let trace_fail_alt = tempdir.join("rinflex.fail_alt.trace");
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
            trace_fail_alt,
            trace_success,
            trace_temp,
            next_id: 0,
        }
    }

    /// Check if the loop should terminate.
    pub fn should_terminate(&self) -> Result<bool, Error> {
        let with_oc = self.attach_constraints_to_trace(&self.trace_fail, 0, &self.constraints)?;
        let _replay = EnvScope::new("LOTTO_REPLAY", &with_oc);
        let _record = EnvScope::new("LOTTO_RECORD", &self.trace_temp);
        let _silent = EnvScope::new("LOTTO_LOGGER_LEVEL", "silent");
        let mut flags = self.flags.clone();
        flags.set_by_opt(&FLAG_REPLAY_GOAL, Value::U64(0));
        let mut bar = ProgressBar::new(self.report_progress, "HALT", self.rounds);
        crate::inflex::always(self.rounds, || {
            let outcome = loop {
                flags.set_by_opt(&flag_seed(), Value::U64(now()));
                let exitcode = checked_execute(&with_oc, &flags, true)?;
                if let Some(outcome) = postexec(&self.trace_temp, exitcode)? {
                    break outcome;
                } else {
                    bar.tick_invalid();
                }
            };
            bar.tick_valid();
            Ok(outcome.is_fail())
        })
    }

    pub fn reset_input(&mut self, replay_goal: Clock) -> Result<(), Error> {
        let _silent = EnvScope::new("LOTTO_LOGGER_LEVEL", "silent");
        let with_oc =
            self.attach_constraints_to_trace(&self.trace_fail, replay_goal, &self.constraints)?;
        trace::replay(&self.flags, &with_oc, &self.trace_fail);
        Ok(())
    }

    /// Do one iteration of the Recursive Inflex main loop.
    ///
    /// It returns [None] when the analysis should terminate.
    pub fn find_next_pair(&mut self) -> Result<Option<PrimitiveConstraint>, Error> {
        let mut symm_set = Vec::new();
        self.inflex_pair(0, 0, &mut symm_set)
    }

    /// Runs inflex on the current trace_fail.
    fn find_inflex(&mut self, min: Clock) -> Result<Option<(Clock, Clock, Event)>, Error> {
        info!("Finding IP...");
        let mut inflex = inflex::Inflex::new_with_flags_and_input(&self.flags, &self.trace_fail);
        inflex.output = self.trace_temp.clone();
        inflex.min = min;
        inflex.report_progress = self.report_progress;
        let ip = inflex.run_fast()?;
        info!("IP is {}", ip);
        if ip == 0 {
            return Ok(None);
        }
        let (event, delta) = self.event_at_clock(&self.flags, &self.trace_fail, ip)?;
        let ip_fixed = ip.wrapping_add_signed(delta as i64).min(ip);
        info!("Source event is\n{}", event.display()?);
        Ok(Some((ip, ip_fixed, event)))
    }

    /// Runs inverseinflex on the current trace_success.
    fn find_inverse_inflex(&mut self, min: Clock) -> Result<Option<(Clock, Clock, Event)>, Error> {
        info!("Finding IIP...");
        let mut inflex = inflex::Inflex::new_with_flags_and_input(&self.flags, &self.trace_success);
        inflex.output = self.trace_temp.clone();
        inflex.report_progress = self.report_progress;
        inflex.min = min;
        let iip = inflex.run_inverse_fast()?;
        info!("IIP is {}", iip);
        let (event, delta) = self.event_at_clock(&self.flags, &self.trace_success, iip)?;
        let iip_fixed = iip.wrapping_add_signed(delta as i64).min(iip);
        info!("Target event is\n{}", event.display()?);
        Ok(Some((iip, iip_fixed, event)))
    }

    /// Check if current set of OCs \/ {pair} is a bug for the prefix trace_fail[:ip-1].
    ///
    /// That is, check if OC \/ {pair} guarantees a failure of the prefix trace_fail[:ip - 1].
    fn sibling_check(&mut self, ip: Clock, pair: &PrimitiveConstraint) -> Result<bool, Error> {
        self.push_virtual_constraint(pair.clone(), true);
        let check_trace =
            self.attach_constraints_to_trace(&self.trace_fail, ip - 1, &self.constraints);
        self.pop_constraint();
        info!("Checking if the pair is a bug for the current set of executions...");
        let check_trace = check_trace?;
        let mut flags = self.flags.to_owned();
        let _replay = EnvScope::new("LOTTO_REPLAY", &check_trace);
        let _record = EnvScope::new("LOTTO_RECORD", &self.trace_temp);
        let _silent = EnvScope::new("LOTTO_LOGGER_LEVEL", "silent");
        let mut bar = ProgressBar::new(self.report_progress, "correct?", self.rounds);
        always(self.rounds, || {
            let outcome = loop {
                flags.set_by_opt(&flag_seed(), Value::U64(now()));
                let exitcode = checked_execute(&check_trace, &flags, true)?;
                if let Some(outcome) = postexec(&self.trace_temp, exitcode)? {
                    break outcome;
                } else {
                    bar.tick_invalid();
                }
            };
            bar.tick_valid();
            Ok(outcome.is_fail())
        })
    }

    /// To discover other IPs, add IIP->IP and find other failures.
    fn essentiality_witness(
        &mut self,
        ip: Clock,
        pair: &PrimitiveConstraint,
    ) -> Result<Option<PathBuf>, Error> {
        self.push_virtual_constraint(pair.flipped(), false);
        let result = self.get_trace(
            Outcome::Fail,
            ip - 1,
            &self.trace_fail,
            &self.trace_fail_alt,
            false,
            false,
        );
        self.pop_constraint();
        match result {
            Ok(()) => Ok(Some(self.trace_fail_alt.to_owned())),
            Err(Error::ExecutionNotFound) => Ok(None),
            Err(e) => Err(e),
        }
    }

    fn inflex_pair(
        &mut self,
        inflex_min: Clock,
        depth: usize,
        symm_set: &mut Vec<PrimitiveConstraint>,
    ) -> Result<Option<PrimitiveConstraint>, Error> {
        info!(
            "Find next inflex pair, depth={}, inflex_min={}",
            depth, inflex_min
        );

        // Find IP.
        let Some((_ip_orig, ip, source)) = self.find_inflex(inflex_min)? else {
            return Ok(None);
        };

        // Find Ts.
        info!(
            "Finding a successful trace up to IP-1={}... #constrs = {}",
            ip - 1,
            self.constraints.len()
        );
        self.get_trace(
            Outcome::Success,
            ip.checked_sub(1).expect("IP cannot be 0"),
            &self.trace_fail,
            &self.trace_success,
            true,
            true,
        )?;

        // Find IIP.
        let Some((_iip_orig, iip, target)) = self.find_inverse_inflex(ip)? else {
            return Ok(None);
        };

        // OC candidate
        let pair = PrimitiveConstraint {
            source: source.clone(),
            target: target.clone(),
            clk: ip - 1,
        };

        // Primitive checking
        let repeated = symm_set.contains(&pair);
        let same_thread = pair.source.t.id == pair.target.t.id;

        // Sibling check
        let correct = !same_thread && (repeated || self.sibling_check(ip, &pair)?);
        if !correct {
            info!("Incorrect");
            self.get_trace(
                Outcome::Fail,
                iip - 1,
                &self.trace_success,
                &self.trace_fail,
                true,
                true,
            )?;
            symm_set.push(pair);
            return self.inflex_pair(iip - 1, depth + 1, symm_set);
        }

        // Essentiality check
        info!("Essentiality check");
        let mut num_added_virt_ocs = 0;
        while let Some(trace_fail_alt) = self.essentiality_witness(ip, &pair)? {
            let (alt_event, _delta) = self.event_at_clock(&self.flags, &trace_fail_alt, ip)?;
            let virt_pair = PrimitiveConstraint {
                source: source.clone(),
                target: alt_event,
                clk: ip - 1,
            };
            info!("Found an essentiality witness\n{}", virt_pair);
            if virt_pair.source.t.id == virt_pair.target.t.id {
                println!("invalid essentiality pair!");
                println!("current failing trace: {}", self.trace_fail.display());
                println!(
                    "alt fail trace (output of EC): {}",
                    trace_fail_alt.display()
                );
                println!("virt_pair is {}", virt_pair);
                println!(
                    "ip = {}, iip = {} (EC replayed up to IP-1={})",
                    ip,
                    iip,
                    ip - 1
                );
                panic!();
            }
            self.push_virtual_constraint(virt_pair, true);
            num_added_virt_ocs += 1;
        }
        info!(
            "Essentiality check done; found {} virtual constraints",
            num_added_virt_ocs
        );

        Ok(Some(pair))
    }

    /// Obtain a new random trace with the expected outcome.
    ///
    /// The execution will obey the currently known ordering
    /// constraints.
    ///
    /// All records from clock 0 to `replay_goal` (inclusive) in
    /// `input` will be replayed exactly.
    pub fn get_trace(
        &self,
        expected_outcome: Outcome,
        replay_goal: Clock,
        input: &Path,
        output: &Path,
        unlimited: bool,
        try_hard: bool,
    ) -> Result<(), Error> {
        info!(
            "get_trace: attaching constraints, #constraints={}",
            self.constraints.len()
        );
        let input = self.attach_constraints_to_trace(input, replay_goal, &self.constraints)?;
        info!(
            "get_trace: input={}, output={}, replay_goal={}",
            input.display(),
            output.display(),
            replay_goal
        );
        let _replay = EnvScope::new("LOTTO_REPLAY", &input);
        let _record = EnvScope::new("LOTTO_RECORD", &output);
        let _env_silent = EnvScope::new("LOTTO_LOGGER_LEVEL", "silent");
        let max_rounds = self.rounds * 10;
        let valid_rounds = if try_hard {
            self.rounds * 10
        } else {
            self.rounds
        };
        let mut flags = self.flags.clone();
        let mut bar = ProgressBar::new(self.report_progress, "", valid_rounds);
        loop {
            if !unlimited
                && (bar.valid_ticks > valid_rounds
                    || bar.valid_ticks == 0 && bar.invalid_ticks > max_rounds)
            {
                warn!("Cannot find satisfying execution in get_trace");
                return Err(Error::ExecutionNotFound);
            }

            flags.set_by_opt(&FLAG_REPLAY_GOAL, Value::U64(replay_goal));
            flags.set_by_opt(&flag_seed(), Value::U64(now()));

            // Restore ichpt. Ichpt_initial was lost whenever we
            // unmarshal a config record, but it is nevertheless
            // required because we accumulate them in Inflex and other
            // procedures. It is incorrect to set this in the first
            // config record because the trace prefix is only
            // meaningful given the same config.
            //
            // Safety: we are trying to explore NEW behaviors after a
            // known prefix. The prefix is not affected by this. Also,
            // more ichpt means more explored behaviors, so soundness
            // is guaranteed. OTOH, if we miss some ichpt, we may not
            // find the desired outcome at all.
            unsafe {
                raw::vec_union(
                    raw::ichpt_initial(),
                    raw::ichpt_final(),
                    Some(raw::ichpt_item_compare),
                );
            }

            let exitcode = checked_execute(&input, &flags, true)?;
            if let Some(actual_outcome) = postexec(&output, exitcode)? {
                bar.tick_valid();
                if expected_outcome == actual_outcome {
                    return Ok(());
                }
            } else {
                bar.tick_invalid();
                continue;
            }
        }
    }

    fn event_at_clock(
        &self,
        flags: &Flags,
        trace: &Path,
        clock: Clock,
    ) -> Result<(Event, i32), Error> {
        let _silent = EnvScope::new("LOTTO_LOGGER_LEVEL", "silent");
        let event = trace::get_event_at_clk(flags, trace, clock)?;

        if matches!(
            event.t.cat,
            Category::CAT_TASK_INIT | Category::CAT_FUNC_EXIT
        ) {
            let after_event = trace::get_event_at_clk(flags, trace, clock + 1)?;
            return Ok((after_event, 1));
        } else if event.t.cat == Category::CAT_BEFORE_CMPXCHG {
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
                return Ok((after_event, 0));
            } else {
                return Ok((event, 0));
            }
        } else if event.t.cat == Category::CAT_AFTER_XCHG {
            // This is in fact similar to CMPXCHG, but they are
            // handled differently, because there is no counterpart to
            // AFTER_CMPXCHG_S and _F events. Here, we make sure we
            // always return the BEFORE event.
            let before_event = trace::get_event_at_clk(flags, trace, clock - 1)?;
            if before_event.t.cat == Category::CAT_BEFORE_XCHG {
                return Ok((before_event, -1));
            } else {
                return Ok((event, 0));
            }
        } else {
            return Ok((event, 0));
        }
    }

    /// Attach constraints to trace through order_enforcer's config.
    ///
    /// The constraints are attached at the clock goal, and any events
    /// following goal+1 will respect the constraints.
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

        handlers::order_enforcer::cli_set_constraints(constraints.clone());

        let mut rec = Trace::load_file(input);
        rec.trim_to_goal(goal, true);
        let r = Record::new_config(goal);
        rec.append(r).expect("append updated oc to trace");
        rec.save(&out);
        // Embed the current constraints into the trace file.  Do not
        // clear constraints for inflex (which will truncate the
        // trace). This sound because (1) a constraint added before
        // its clk does no harm, and (2) a constraint duplicated after
        // its clock will be deduped.

        Ok(out)
    }

    /// Update `constraints_to_update` using records from clock 0 to
    /// clock `goal` (inclusive).
    #[allow(dead_code)]
    pub fn update_constraints(
        &self,
        input: &Path,
        goal: Clock,
        constraints_to_update: &ConstraintSet,
    ) -> Result<ConstraintSet, Error> {
        trace::expand(&self.flags, input, &self.trace_temp, Some(goal));
        let mut constraints = constraints_to_update.clone();
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
            // `cnt` is meta. It counts the number of occurrences of (event-except-cnt).
            for constraint in constraints.iter_mut() {
                if constraint.c.target.equals(&event) {
                    constraint.c.target.cnt = constraint.c.target.cnt.saturating_sub(1);
                    if constraint.c.target.cnt == 0 && constraint.c.source.cnt != 0 {
                        panic!(
                            "Source must have been satisfied, source={}, target={}",
                            constraint.c.source, constraint.c.target
                        );
                    }
                }
                if constraint.c.source.equals(&event) {
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

    pub fn push_real_constraint(&mut self, c: PrimitiveConstraint) {
        let id = self.get_id();
        let constraint = Constraint {
            c,
            virt: false,
            positive: false,
            id,
        };
        self.constraints.push(constraint);
    }

    fn push_virtual_constraint(&mut self, c: PrimitiveConstraint, positive: bool) {
        let id = self.get_id();
        let constraint = Constraint {
            c,
            virt: true,
            positive,
            id,
        };
        self.constraints.push(constraint);
    }

    fn pop_constraint(&mut self) -> Option<Constraint> {
        self.constraints.pop()
    }

    fn get_id(&mut self) -> usize {
        let id = self.next_id;
        self.next_id += 1;
        id
    }
}

/// For debugging.
#[allow(dead_code)]
fn pause(prompt: &str) -> String {
    use std::io::Write;
    print!("{}", prompt);
    std::io::stdout().flush().unwrap();
    let mut s = String::new();
    std::io::stdin().read_line(&mut s).unwrap();
    s
}
