
use lotto::log::*;

pub use handler_await_address;
pub use handler_perf;
pub use handler_spin_loop;

#[cfg(feature = "stable_address_map")]
pub use handler_rinflex;

/// Add all handlers into the list of known handlers.
pub fn register() {
    trace!("Initializing Rust handlers.");

    handler_perf::register();
    handler_await_address::register();
    handler_spin_loop::register();

    // Rinflex handlers
    #[cfg(feature = "stable_address_map")]
    handler_rinflex::register();
}

pub fn register_flags() {
    #[cfg(feature = "stable_address_map")]
    handler_rinflex::register_flags();
}
