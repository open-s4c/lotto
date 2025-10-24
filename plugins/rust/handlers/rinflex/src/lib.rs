//! # rinflex
//!
//! The engine part of the "recursive inflex" algorithm.

use rinflex::handlers;

pub fn register() {
    handlers::stacktrace::register();
    handlers::address::register();
    handlers::cas::register();
    handlers::event::register();
    handlers::order_enforcer::register();
}

pub fn register_flags() {
    handlers::stacktrace::register_flags();
    handlers::address::register_flags();
    handlers::cas::register_flags();
    handlers::event::register_flags();
    handlers::order_enforcer::register_flags();
}
