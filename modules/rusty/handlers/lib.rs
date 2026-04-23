use lotto::log::*;

pub use handler_await_address;
pub use handler_perf;
pub use handler_rinflex;
pub use handler_spin_loop;

/// Add all handlers into the list of known handlers.
pub fn register() {
    trace!("Initializing Rust handlers.");

    handler_perf::register();
    handler_await_address::register();
    handler_spin_loop::register();

    // Rinflex handlers
    handler_rinflex::register();
}

pub fn register_flags() {
    handler_await_address::register_flags();
    handler_spin_loop::register_flags();

    handler_rinflex::register_flags();
}
