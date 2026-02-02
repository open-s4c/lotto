use crate::cli::Args;
use crate::owned::Owned;
use std::ffi::c_void;

use lotto_sys as raw;

crate::wrap_with_dispose!(Record, raw::record_t, raw::sys_free, *mut c_void);

/// Type of a clock of a record in a trace file.
pub type Clock = raw::clk_t;

impl Record {
    pub fn new(payload_size: usize) -> Owned<Record> {
        let ptr = unsafe { raw::record_alloc(payload_size) };
        unsafe { Owned::from_raw(ptr) }
    }

    pub fn new_config(goal: Clock) -> Owned<Record> {
        let ptr = unsafe { raw::record_config(goal) };
        unsafe { Owned::from_raw(ptr) }
    }

    pub fn args(&self) -> &Args {
        let ptr = unsafe { raw::record_args(self.as_ptr()) };
        unsafe { Args::wrap(ptr) }
    }

    pub fn unmarshal(&self) {
        crate::brokers::statemgr::record_unmarshal(self);
    }
}

impl ToOwned for Record {
    type Owned = Owned<Record>;

    fn to_owned(&self) -> Self::Owned {
        unsafe {
            let cloned = raw::record_clone(self.as_ptr());
            Owned::from_raw(cloned)
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn clone() {
        let mut foo = Record::new(0);
        foo.id = 1;
        foo.clk = 114514;
        foo.kind = raw::record::RECORD_CONFIG;
        let bar = foo.clone();
        assert_eq!(foo.id, bar.id);
        assert_eq!(foo.clk, bar.clk);
        assert_eq!(foo.kind, bar.kind);
    }
}
