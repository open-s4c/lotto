use lotto::base::{Flags, Prng, Trace};
use lotto::cli::flags::*;
use lotto::engine::flags::*;
use lotto::raw;
use std::path::Path;

use crate::error::Error;
use crate::handlers;
use crate::stats::STATS;

/// A execution whose outcome is checked.
pub struct Exec<'a, F> {
    pub input: &'a Path,
    pub output: &'a Path,
    pub flags: &'a Flags,
    pub exitcode: Option<i32>,
    pub filter: Option<F>,
}

impl<'a> Exec<'a, fn(&Path) -> bool> {
    pub fn new(input: &'a Path, output: &'a Path, flags: &'a Flags) -> Self {
        Exec {
            input,
            output,
            flags,
            exitcode: None,
            filter: None,
        }
    }
}

impl<'a, F> Exec<'a, F> {
    pub fn new_with_filter(input: &'a Path, output: &'a Path, flags: &'a Flags, filter: F) -> Self {
        Exec {
            input,
            output,
            flags,
            exitcode: None,
            filter: Some(filter),
        }
    }

    /// Execute and set internal states.
    pub fn exec(&mut self) {
        let mut rec = Trace::load_file(self.input);
        let first = rec.next(raw::record::RECORD_START).expect("first record");
        let args = first.args();
        first.unmarshal();
        unsafe {
            raw::exec_info_replay_envvars(self.flags.get_uval(&FLAG_VERBOSE) as i32);
        }
        // Make sure `execute` uses the seed from `flags`, as it might
        // append another config record which uses the current engine's
        // seed.
        let seed = self.flags.get_value(&flag_seed()).as_u64();
        Prng::current_mut().seed = seed as u32;
        let exitcode = {
            let exec_start_instant = std::time::Instant::now();
            let exitcode = lotto::cli::execute(args, self.flags, /* config */ true);
            STATS.tick_run();
            STATS.count_run_time(exec_start_instant);
            exitcode
        };
        self.exitcode = Some(exitcode);
    }
}

impl<'a, F> Exec<'a, F>
where
    F: Fn(&Path) -> bool,
{
    /// Check the outcome.
    ///
    /// Returns None if the execution should be discarded.
    pub fn postexec(&mut self) -> Result<Option<Outcome>, Error> {
        let exitcode = self
            .exitcode
            .expect("exec() should be called before postexec()");

        let mut rec = Trace::load_file(self.output);
        let last = rec.last().expect("no last record");

        // Lotto crashed?
        if last.reason.is_runtime() {
            return Err(Error::LottoError {
                input: self.input.to_path_buf(),
                output: self.output.to_path_buf(),
            });
        }

        // Should ignore? Probably from handler_drop.
        if last.reason == raw::reason::REASON_IGNORE {
            STATS.tick_discarded_run();
            return Ok(None);
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
                STATS.tick_discarded_run();
                return Ok(None);
            }
        }

        // Check if some handler aborts the execution, which is considered
        // a failure, but exitcode may be 0.
        if last.reason.is_abort() {
            return Ok(Some(Outcome::Fail));
        }

        // User-supplied filter.
        if let Some(ref mut filter) = self.filter {
            if !filter(self.output) {
                return Ok(None);
            }
        }

        // All tests passed.
        if exitcode == 0 {
            Ok(Some(Outcome::Success))
        } else {
            Ok(Some(Outcome::Fail))
        }
    }

    // Exec and postexec combined.
    pub fn exec_and_check(&mut self) -> Result<Option<Outcome>, Error> {
        self.exec();
        self.postexec()
    }
}

pub fn preload(flags: &Flags) {
    lotto::cli::preload(
        flags.get_sval(&FLAG_TEMPORARY_DIRECTORY),
        flags.get_uval(&FLAG_VERBOSE),
        !flags.is_on(&FLAG_NO_PRELOAD),
        flags.get_sval(&flag_memmgr_runtime()),
        flags.get_sval(&flag_memmgr_user()),
    );
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Outcome {
    Success,
    Fail,
}

impl Outcome {
    pub fn is_fail(self) -> bool {
        matches!(self, Outcome::Fail)
    }

    pub fn is_success(self) -> bool {
        matches!(self, Outcome::Success)
    }
}
