use std::path::{Path, PathBuf};

use lotto::base::{Clock, Flags, Trace};
use lotto::base::{EnvScope, Value};
use lotto::cli::flags::*;
use lotto::cli::prng;
use lotto::engine::flags::*;
use lotto::log::*;
use lotto::owned::Owned;

use crate::error::Error;
use crate::exec::Exec;
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
    pub report_progress: bool,

    //
    // debug
    //
    pub debug: bool,
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

        Inflex {
            input: input.to_owned(),
            output,
            flags: flags.to_owned(),
            last_clk,
            candidate,
            temp_output,
            report_progress: true,
            rounds,
            min: 0,
            debug: flags.get_uval(&FLAG_VERBOSE) >= 6,
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

    pub fn set_log_level(&self) -> EnvScope {
        if self.debug {
            EnvScope::new("LOTTO_LOGGER_LEVEL", "debug")
        } else {
            EnvScope::new("LOTTO_LOGGER_LEVEL", "silent")
        }
    }

    /// Find the inverse inflection point.
    pub fn run_inverse(&self) -> Result<Clock, Error> {
        let mut flags = self.flags.clone();

        let _env_replay = EnvScope::new("LOTTO_REPLAY", &self.input);
        let _env_record = EnvScope::new("LOTTO_RECORD", &self.temp_output);
        let _logger = self.set_log_level();

        // If it's unlikely to hit the bug, the probablistical
        // algorithm will just return 0. Use a slower but perhaps
        // stabler algorithm here.
        binary_search(self.min, self.last_clk, |clk| {
            let mut bar = ProgressBar::new(self.report_progress, "", self.rounds);
            bar.set_prefix_string(format!("IIP={}", clk));
            flags.set_by_opt(&FLAG_REPLAY_GOAL, Value::U64(clk));
            let success_forever = always(self.rounds, || loop {
                flags.set_by_opt(&flag_seed(), Value::U64(prng::next()));
                let mut exec = Exec::new(&self.input, &self.temp_output, &flags);
                if let Some(outcome) = exec.exec_and_check()? {
                    bar.tick_valid();
                    return Ok(outcome.is_success());
                } else {
                    bar.tick_invalid();
                }
            })?;
            Ok(success_forever)
        })
    }

    pub fn run_inverse_fast(&self) -> Result<Clock, Error> {
        let mut flags = self.flags.clone();

        let mut iip = self.min;
        let mut last_iip = 0;
        let mut confidence = 0;

        if self.report_progress {
            info!("inverse inflex input = {}", self.input.display());
        }

        let _env_replay = EnvScope::new("LOTTO_REPLAY", &self.input);
        let _env_record = EnvScope::new("LOTTO_RECORD", &self.temp_output);
        let _logger = self.set_log_level();

        let mut bar = ProgressBar::new(self.report_progress, "", self.rounds);
        while confidence <= self.rounds && iip < self.last_clk {
            iip = binary_search(iip, self.last_clk, |clk| {
                flags.set_by_opt(&FLAG_REPLAY_GOAL, Value::U64(clk));
                loop {
                    flags.set_by_opt(&flag_seed(), Value::U64(prng::next()));
                    let mut exec = Exec::new(&self.input, &self.temp_output, &flags);
                    if let Some(outcome) = exec.exec_and_check()? {
                        return Ok(outcome.is_success());
                    } else {
                        bar.tick_invalid();
                    }
                }
            })?;

            if iip == last_iip {
                confidence += 1;
                bar.tick_valid();
            } else {
                last_iip = iip;
                confidence = 1;
                let mut rec = Trace::load_file(&self.input);
                rec.truncate(&self.candidate, iip);

                // Start a new progress bar.  The idea is to keep a
                // trace of attempted clocks on the terminal.
                bar = ProgressBar::new(self.report_progress, "", self.rounds);
                bar.set_prefix_string(format!("IIP={}", iip));
            }
        }

        Ok(iip)
    }

    /// Find the (normal) inflection point using probablistic binary
    /// search.
    pub fn run(&self) -> Result<Clock, Error> {
        let mut flags = self.flags.clone();

        let _env_replay = EnvScope::new("LOTTO_REPLAY", &self.input);
        let _env_record = EnvScope::new("LOTTO_RECORD", &self.temp_output);
        let _logger = self.set_log_level();

        binary_search(self.min, self.last_clk, |clk| {
            let mut bar = ProgressBar::new(self.report_progress, "", self.rounds);
            bar.set_prefix_string(format!("IP={}", clk));
            flags.set_by_opt(&FLAG_REPLAY_GOAL, Value::U64(clk));
            let fail_forever = always(self.rounds, || loop {
                flags.set_by_opt(&flag_seed(), Value::U64(prng::next()));
                let mut exec = Exec::new(&self.input, &self.temp_output, &flags);
                if let Some(outcome) = exec.exec_and_check()? {
                    bar.tick_valid();
                    return Ok(outcome.is_fail());
                } else {
                    bar.tick_invalid();
                }
            })?;
            Ok(fail_forever)
        })
    }

    /// The fast probablistic binary search.
    pub fn run_fast(&self) -> Result<Clock, Error> {
        let mut flags = self.flags.clone();

        let mut ip = self.min;
        let mut last_ip = 0;
        let mut confidence = 0;

        if self.report_progress {
            info!("inflex input = {}", self.input.display());
        }

        let _env_replay = EnvScope::new("LOTTO_REPLAY", &self.input);
        let _env_record = EnvScope::new("LOTTO_RECORD", &self.temp_output);
        let _logger = self.set_log_level();

        let mut bar = ProgressBar::new(self.report_progress, "", self.rounds);
        while confidence <= self.rounds && ip < self.last_clk {
            ip = binary_search(ip, self.last_clk, |clk| {
                flags.set_by_opt(&FLAG_REPLAY_GOAL, Value::U64(clk));
                loop {
                    flags.set_by_opt(&flag_seed(), Value::U64(prng::next()));
                    let mut exec = Exec::new(&self.input, &self.temp_output, &flags);
                    if let Some(outcome) = exec.exec_and_check()? {
                        return Ok(outcome.is_fail());
                    } else {
                        bar.tick_invalid();
                    }
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
