use lotto_sys as raw;

/// Returns the next random number.
pub fn next() -> u64 {
    unsafe { raw::prng_next() }
}
