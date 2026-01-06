use crate::base::Flags;
use lotto_sys as raw;

pub fn print_flags(flags: &Flags) {
    unsafe {
        raw::flags_print(flags.as_ptr());
    }
}
