use crate::base::Flag;
use lotto_sys as raw;

macro_rules! simple_wrap_raw {
    ($name:ident, $ty:tt) => {
        pub fn $name() -> $ty {
            unsafe { raw::$name() }
        }
    };
}

simple_wrap_raw!(flag_strategy, Flag);

simple_wrap_raw!(flag_record_granularity, Flag);

simple_wrap_raw!(flag_stable_address_method, Flag);

simple_wrap_raw!(flag_seed, Flag);

simple_wrap_raw!(flag_termination_type, Flag);
simple_wrap_raw!(flag_termination_limit, Flag);

simple_wrap_raw!(flag_memmgr_runtime, Flag);
simple_wrap_raw!(flag_memmgr_user, Flag);
