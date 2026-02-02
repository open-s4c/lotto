use lotto_sys as raw;

crate::wrap!(Prng, raw::prng_t);

impl Prng {
    /// Retrieve the current global PRNG instance.
    pub fn current() -> &'static Prng {
        unsafe { Prng::wrap(raw::prng()) }
    }

    /// Retrieve the current global PRNG instance.
    pub fn current_mut() -> &'static mut Prng {
        unsafe { Prng::wrap_mut(raw::prng()) }
    }
}
