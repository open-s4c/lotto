use std::mem::MaybeUninit;

use lotto_sys::{self as raw, sys_clock_gettime};

/// Retrieve a nanosecond timestamp.
pub fn now() -> u64 {
    let ts = unsafe {
        let mut ts: MaybeUninit<raw::timespec> = MaybeUninit::uninit();
        let r = sys_clock_gettime(libc::CLOCK_MONOTONIC, ts.as_mut_ptr());
        if r != 0 {
            panic!("Could not get clock time");
        }
        ts.assume_init()
    };
    let seconds_ns = ts.tv_sec as u64 * 1000 * 1000 * 1000;
    let nanosceonds_ns = ts.tv_nsec as u64;
    seconds_ns + nanosceonds_ns
}
