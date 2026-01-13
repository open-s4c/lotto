use std::path::{Path, PathBuf};

use lotto::base::{Clock, Flags, Trace};
use lotto::base::{EnvScope, Value};
use lotto::cli::execute;
use lotto::cli::flags::*;
use lotto::engine::flags::*;
use lotto::log::*;
use lotto::owned::Owned;
use lotto::raw;

use crate::error::Error;
use crate::handlers;
use crate::progress::ProgressBar;

pub struct Inflex {
    input: PathBuf,
    pub output: PathBuf,

    //
    // input
    // will be determined on construction
    //
    pub flags: Owned<Flags>,
    pub last_clk: Clock,

    //
    // temporary files
    //
    pub temp_output: PathBuf,
    pub candidate: PathBuf,

    //
    // search
    //
    pub rounds: u64,
    pub min: Clock,

    //
    // other parameters
    //
    pub crep: bool,
    pub report_progress: bool,
}

impl Inflex {
    /// Create an Inflex request using parameters specified by the
    /// lotto flags.
    ///
    /// This is equivalent to calling [`new_with_flags_and_input`]
    /// with `FLAG_INPUT`.
    pub fn new_with_flags(flags: &Flags) -> Inflex {
        let input = flags.get_path_buf(&FLAG_INPUT);
        Self::new_with_flags_and_input(flags, &input)
    }

    /// Create an Inflex request using parameters specified by the
    /// lotto flags and the input trace.
    pub fn new_with_flags_and_input(flags: &Flags, input: &Path) -> Inflex {
        let mut rec = Trace::load_file(input.to_str().unwrap());
        let last = rec.last().expect("Last record in the input trace");
        let last_clk = last.clk;

        let output = flags.get_path_buf(&FLAG_OUTPUT);

        let tempdir = flags.get_path_buf(&FLAG_TEMPORARY_DIRECTORY);
        let candidate = tempdir.join("inflex.candidate.trace");
        let temp_output = tempdir.join("inflex.temp.trace");

        let rounds = flags.get_uval(&FLAG_ROUNDS);
        let crep = flags.is_on(&FLAG_CREP);

        Inflex {
            input: input.to_owned(),
            output,
            flags: flags.to_owned(),
            last_clk,
            candidate,
            temp_output,
            report_progress: true,
            crep,
            rounds,
            min: 0,
        }
    }

    /// Reset the input trace and other input-related arguments.
    pub fn set_input(&mut self, input: &Path) {
        let mut rec = Trace::load_file(input.to_str().unwrap());
        let last = rec.last().expect("Last record in the input trace");
        let last_clk = last.clk;
        self.input = input.to_owned();
        self.last_clk = last_clk;
    }

    pub fn input(&self) -> &Path {
        &self.input
    }

    /// Find the inverse inflection point.
    pub fn run_inverse(&self) -> Result<Clock, Error> {
        let mut flags = self.flags.clone();

        if self.crep {
            lotto::crep::backup_make();
        }

        let _env_replay = EnvScope::new("LOTTO_REPLAY", &self.input);
        let _env_record = EnvScope::new("LOTTO_RECORD", &self.temp_output);
        let _env_silent = EnvScope::new("LOTTO_LOGGER_LEVEL", "silent");

        // If it's unlikely to hit the bug, the probablistical
        // algorithm will just return 0. Use a slower but perhaps
        // stabler algorithm here.
        binary_search(self.min, self.last_clk, |clk| {
            let mut bar = ProgressBar::new(self.report_progress, "", self.rounds);
            bar.set_prefix_string(format!("IIP={}", clk));
            flags.set_by_opt(&FLAG_REPLAY_GOAL, Value::U64(clk));
            let success_forever = always(self.rounds, || {
                if self.crep {
                    lotto::crep::backup_restore();
                }
                loop {
                    flags.set_by_opt(&flag_seed(), Value::U64(lotto::sys::now()));
                    let exitcode = checked_execute(&self.input, &flags, true)?;
                    if should_discard_execution(&self.temp_output)? {
                        bar.tick_invalid();
                        continue;
                    }
                    bar.tick_valid();
                    return Ok(exitcode == 0);
                }
            })?;
            Ok(success_forever)
        })
    }

    /// Find the (normal) inflection point using probablistic binary
    /// search.
    pub fn run(&self) -> Result<Clock, Error> {
        let mut flags = self.flags.clone();

        if self.crep {
            lotto::crep::backup_make();
        }

        let _env_replay = EnvScope::new("LOTTO_REPLAY", &self.input);
        let _env_record = EnvScope::new("LOTTO_RECORD", &self.temp_output);
        let _env_silent = EnvScope::new("LOTTO_LOGGER_LEVEL", "silent");

        binary_search(self.min, self.last_clk, |clk| {
            let mut bar = ProgressBar::new(self.report_progress, "", self.rounds);
            bar.set_prefix_string(format!("IP={}", clk));
            flags.set_by_opt(&FLAG_REPLAY_GOAL, Value::U64(clk));
            let fail_forever = always(self.rounds, || {
                if self.crep {
                    lotto::crep::backup_restore();
                }
                loop {
                    flags.set_by_opt(&flag_seed(), Value::U64(lotto::sys::now()));
                    let exitcode = checked_execute(&self.input, &flags, true)?;
                    if should_discard_execution(&self.temp_output)? {
                        bar.tick_invalid();
                        continue;
                    }
                    bar.tick_valid();
                    return Ok(exitcode != 0);
                }
            })?;
            Ok(fail_forever)
        })
    }

    /// The fast probablistic binary search.
    pub fn run_fast(&self) -> Result<Clock, Error> {
        let mut flags = self.flags.clone();

        if self.crep {
            lotto::crep::backup_make();
        }

        let mut ip = self.min;
        let mut last_ip = 0;
        let mut confidence = 0;

        if self.report_progress {
            info!("inflex input = {}", self.input.display());
        }

        let _env_replay = EnvScope::new("LOTTO_REPLAY", &self.input);
        let _env_record = EnvScope::new("LOTTO_RECORD", &self.temp_output);
        let _env_silent = EnvScope::new("LOTTO_LOGGER_LEVEL", "silent");

        let mut bar = ProgressBar::new(self.report_progress, "", self.rounds);
        while confidence <= self.rounds && ip < self.last_clk {
            ip = binary_search(ip, self.last_clk, |clk| {
                flags.set_by_opt(&FLAG_REPLAY_GOAL, Value::U64(clk));
                loop {
                    flags.set_by_opt(&flag_seed(), Value::U64(lotto::sys::now()));
                    if self.crep {
                        lotto::crep::backup_restore();
                    }
                    let return_code = checked_execute(&self.input, &flags, true)?;
                    if should_discard_execution(&self.temp_output)? {
                        bar.tick_invalid();
                        continue;
                    }
                    return Ok(return_code != 0);
                }
            })?;

            if ip == last_ip {
                confidence += 1;
                bar.tick_valid();
            } else {
                last_ip = ip;
                confidence = 1;
                let mut rec = Trace::load_file(&self.input);
                rec.truncate(&self.candidate, ip);

                // Start a new progress bar.  The idea is to keep a
                // trace of attempted clocks on the terminal.
                bar = ProgressBar::new(self.report_progress, "", self.rounds);
                bar.set_prefix_string(format!("IP={}", ip));
            }
        }

        Ok(ip)
    }
}

/// Generic binary search.
///
/// - If `pred` returns `true`, cut the right half.
/// - If `pred` returns `false`, cut the left half.
pub fn binary_search(
    mut l: u64,
    mut r: u64,
    mut pred: impl FnMut(u64) -> Result<bool, Error>,
) -> Result<u64, Error> {
    while r > l {
        let x = l + (r - l) / 2;
        if pred(x)? {
            r = x;
        } else {
            l = x + 1;
        }
    }
    Ok(r)
}

/// Check if `pred` is always true, probablistically.
pub fn always(rounds: u64, mut pred: impl FnMut() -> Result<bool, Error>) -> Result<bool, Error> {
    for _ in 0..rounds {
        if !pred()? {
            return Ok(false);
        }
    }
    Ok(true)
}

/// [`execute`] but guard against `SIGINT`.
pub fn checked_execute(trace: &Path, flags: &Flags, config: bool) -> Result<i32, Error> {
    let mut rec = Trace::load_file(trace);
    let first = rec.next(raw::record::RECORD_START).expect("first record");
    let args = first.args();
    first.unmarshal();
    unsafe {
        raw::exec_info_replay_envvars();
    }
    let exitcode = execute(args, flags, config);
    if exitcode == 130 {
        return Err(Error::Interrupted);
    }
    return Ok(exitcode);
}

/// Check if this execution should be discarded.
pub fn should_discard_execution(output: &Path) -> Result<bool, Error> {
    let mut rec = Trace::load_file(output);
    let last = rec.last().expect("last record");

    // Lotto crashed?
    if last.reason.is_runtime() {
        return Err(Error::LottoError);
    }

    // Should ignore? Probably from handler_drop.
    if last.reason == raw::reason::REASON_IGNORE {
        return Ok(true);
    }

    // Check the FINAL record and check whether order_enforcer wants
    // us to discard.
    handlers::order_enforcer::cli_reset();
    if last.kind == raw::record::RECORD_EXIT && last.size > 0 {
        unsafe {
            raw::statemgr_unmarshal(
                last.data.as_ptr() as *const std::ffi::c_void,
                raw::state_type::STATE_TYPE_FINAL,
                true,
            );
        }
        if handlers::order_enforcer::should_discard() {
            return Ok(true);
        }
    }

    // All tests passed.
    Ok(false)
}

pub fn preload(flags: &Flags) {
    lotto::cli::preload(
        flags.get_sval(&FLAG_TEMPORARY_DIRECTORY),
        flags.is_on(&FLAG_VERBOSE),
        !flags.is_on(&FLAG_NO_PRELOAD),
        flags.is_on(&FLAG_CREP),
        false,
        flags.get_sval(&flag_memmgr_runtime()),
        flags.get_sval(&flag_memmgr_user()),
    );
}
