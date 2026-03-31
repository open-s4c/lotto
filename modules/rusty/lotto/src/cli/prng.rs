use std::sync::atomic::{AtomicU64, Ordering};

/// A simple PRNG. Not cryptographically secure.
///
/// The algorithm is provided by POSIX.1-2001.
pub fn next() -> u64 {
    static SEED: AtomicU64 = AtomicU64::new(1);
    let seed = SEED.load(Ordering::Relaxed);
    let next = seed.wrapping_mul(1103515245).wrapping_add(12345);
    SEED.store(next, Ordering::Relaxed);
    next
}
