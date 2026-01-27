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

    /// Do one iteration of the Recursive Inflex main loop.
    ///
    /// It returns [None] when the analysis should terminate.
    pub fn find_next_pair(&mut self) -> Result<Option<PrimitiveConstraint>, Error> {
        let mut symm_set = Vec::new();
        self.inflex_pair(0, 0, &mut symm_set)
    }

    /// Runs inflex on the current trace_fail.
    fn find_inflex(&mut self, min: Clock) -> Result<Option<(Clock, Event)>, Error> {
        info!("Finding IP...");
        let updated = self
            .unsafely_set_constraints_in_first_config_record(&self.trace_fail, &self.constraints)?;
        let mut inflex = inflex::Inflex::new_with_flags_and_input(&self.flags, &updated);
        inflex.output = self.trace_temp.clone();
        inflex.min = min;
        inflex.report_progress = self.report_progress;
        let ip = inflex.run_fast()?;
        info!("IP is {}", ip);
        if ip == 0 {
            return Ok(None);
        }
        let (event, delta) = self.event_at_clock(&self.flags, &self.trace_fail, ip)?;
        info!("Source event is\n{}", event.display()?);
        Ok(Some((ip.wrapping_add_signed(delta as i64), event)))
    }

    /// Runs inverseinflex on the current trace_success.
    fn find_inverse_inflex(&mut self, min: Clock) -> Result<Option<(Clock, Event)>, Error> {
        info!("Finding IIP...");
        let updated = self.unsafely_set_constraints_in_first_config_record(
            &self.trace_success,
            &self.constraints,
        )?;
        let mut inflex = inflex::Inflex::new_with_flags_and_input(&self.flags, &updated);
        inflex.output = self.trace_temp.clone();
        inflex.report_progress = self.report_progress;
        inflex.min = min;
        let iip = inflex.run_inverse_fast()?;
        info!("IIP is {}", iip);
        let (event, delta) = self.event_at_clock(&self.flags, &self.trace_success, iip)?;
        info!("Target event is\n{}", event.display()?);
        Ok(Some((iip.wrapping_add_signed(delta as i64), event)))
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
        let Some((ip, source)) = self.find_inflex(inflex_min)? else {
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
        let Some((iip, target)) = self.find_inverse_inflex(ip)? else {
            return Ok(None);
        };

        // OC candidate
        let pair = PrimitiveConstraint {
            source: source.clone(),
            target: target.clone(),
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

        if event.t.cat == Category::CAT_BEFORE_CMPXCHG {
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

    /// Returns the output trace file, as well as the last clock of
    /// the output trace. The clock corresponds to the last event
    /// which is not affected by the constraints.
    ///
    /// This is the unsafe version. That is, it doesn't try to
    /// identify whether or not the suffix respects the ordering
    /// constraints. Rather, it just modifies the config.
    pub fn unsafely_set_constraints_in_first_config_record(
        &self,
        input: &Path,
        constraints: &ConstraintSet,
    ) -> Result<PathBuf, Error> {
        let input_filename = input.file_name().expect("must be a file").to_str().unwrap();
        let output_path = self.tempdir.join(format!("{}+oc", input_filename));
        let mut input = Trace::load_file(input);
        let mut st = Stream::new_file_out(&output_path);
        let mut output = Trace::new_file(&mut st);
        while let Some(r) = input.next_any() {
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
        Ok(output_path)
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

    /// Update `constraints_to_update` using records from clock 0 to
    /// clock `goal` (inclusive).
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
