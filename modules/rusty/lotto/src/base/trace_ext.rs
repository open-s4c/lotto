use std::path::Path;

use crate::base::trace::*;
use crate::base::{Clock, Record};
use lotto_sys as raw;

impl Trace {
    /// Truncate the trace to a specific clock (inclusive).
    pub fn truncate<P: AsRef<Path>>(&mut self, output: P, target: Clock) -> Clock {
        while self.last().is_some_and(|last| last.clk > target) {
            self.forget();
        }
        let last = self.last().unwrap();
        let last_clk = last.clk;
        if last_clk < target {
            let mut r = Record::new(0);
            r.clk = target;
            r.kind = raw::record::RECORD_OPAQUE;
            self.append(r).expect("Failed to append an OPAQUE record");
        }
        self.save(output);
        last_clk
    }
}
