use lotto::base::envvar::EnvScope;
use lotto::base::flags::Flags;
use lotto::base::record_granularity::*;
use lotto::base::{Clock, Record, TaskId, Trace, Value};
use lotto::cli::flags::FLAG_REPLAY_GOAL;
use lotto::engine::flags::*;
use lotto::raw;
use std::path::Path;

use crate::error::Error;
use crate::handlers;
use crate::Event;

/// Get the last clock of a trace.
pub fn get_last_clk(input: &Path) -> Clock {
    let mut rec = Trace::load_file(input);
    let mut cur;
    let mut res = 0;
    while {
        cur = rec.next_any();
        cur.is_some()
    } {
        res = cur.unwrap().clk;
        rec.advance();
    }
    res
}

/// A basic replay.
///
/// # Note
///
/// For this to work, [`lotto::preload`] must be called.
pub fn replay(flags: &Flags, input: &Path, output: &Path) -> i32 {
    let _replay = EnvScope::new("LOTTO_REPLAY", input);
    let _record = EnvScope::new("LOTTO_RECORD", output);
    let mut rec = Trace::load_file(input);
    let first = rec
        .next(lotto::raw::record::RECORD_START)
        .expect("trace should have some records");
    let args = first.args();

    lotto::cli::execute(&args, flags, true)
}

/// Expand trace to granularity CAPTURE until clk.
pub fn expand(flags: &Flags, input: &Path, output: &Path, clk: Option<Clock>) -> i32 {
    let mut flags = flags.to_owned();
    let _granularity = EnvScope::new(
        "LOTTO_RECORD_GRANULARITY",
        RecordGranularities::from(RECORD_GRANULARITY_CAPTURE).to_string(),
    );
    if let Some(clk) = clk {
        flags.set_by_opt(&FLAG_REPLAY_GOAL, Value::U64(clk));
        flags.set_by_opt(
            &flag_termination_type(),
            Value::U64(raw::termination_mode::TERMINATION_MODE_REPLAY as u64),
        );
    }
    replay(&flags, input, output)
}

/// Get an event in the trace file at a specified clock.
///
/// Note, this will modify the global states.
pub fn get_event_at_clk(flags: &Flags, file: &Path, clk: Clock) -> Result<Event, Error> {
    use lotto::cli::flags::*;

    let tempdir = flags.get_path_buf(&FLAG_TEMPORARY_DIRECTORY);
    let full_trace_path = tempdir.join("full_trace_for_finding_event");

    expand(flags, file, &full_trace_path, Some(clk));

    if !full_trace_path.exists() {
        panic!("Cannot get the full trace of {}", file.display());
    }

    let mut rec = Trace::load_file(&full_trace_path);
    let mut cur: Option<&Record>;
    while {
        cur = rec.next_any();
        cur.is_some_and(|cur| clk > cur.clk)
    } {
        rec.advance();
    }
    let cur = match cur {
        Some(r) if r.clk == clk => r,
        _ => return Err(Error::ClockNotFound(full_trace_path.clone(), clk)),
    };
    cur.unmarshal();
    match handlers::event::get_event(TaskId::new(cur.id)) {
        Some(mut e) => {
            // Update the clock to match the trace.
            //
            // It's possible that e.clk < clk, because e.clk is the
            // clock when it's captured, and r.clk is the clock when
            // this event actually happened ('resumed').
            e.clk = clk;
            Ok(e)
        }
        None => return Err(Error::EventNotFound(full_trace_path.clone(), clk)),
    }
}
