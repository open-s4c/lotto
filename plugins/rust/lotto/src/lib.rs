//

#![allow(clippy::result_unit_err)]

// Re-export lotto_macros and supporting symbols
pub use lotto_macros::*;
pub mod reexport {
    //! # Re-exports for [lotto_macros]
    pub use once_cell::sync::Lazy;
}

// Re-export lotto_sys
pub mod raw {
    //! # Unsafe low-level bindings to Lotto C APIs
    pub use lotto_sys::*;
}

// Re-export lotto_log
pub mod log {
    //! # Logging
    //!
    //! `debug!`, `info!`, `warn!`, and `error!` map to the Lotto
    //! counterparts directly, and these logs will be delivered to the
    //! Lotto logging system.
    //!
    //! ## Handling of `trace!`
    //!
    //! `trace!` is not part of Lotto's standard logging levels.  It's
    //! redefined to have be controlled by an environmental variable
    //! `LOTTO_RUST_TRACE`.
    //!
    //! The input of `LOTTO_RUST_TRACE` is a comma-separated list of
    //! `REGEX=TYPE` or `TYPE`, where only trace messages from targets
    //! matching `REGEX` are redirected according to `TYPE`. `TYPE`
    //! can be one of the following:
    //!
    //! - `discard`: Discard completely.
    //! - `stderr`: Redirect to stderr, regardless of other logging levels.
    //! - `debug`: Redirect to `debug!`.
    //! - `FILENAME`: Redirect to FILENAME.
    //!
    //! The first matching type takes effect.
    //!
    //! Examples:
    //!
    //! - `LOTTO_RUST_TRACE=`: discard all messages.
    //! - `LOTTO_RUST_TRACE=handler_.*=stderr`: print trace messages from
    //!   `handler_.*` to stderr.
    //!
    //! ## Panic Hook
    //!
    //! Set envvar `RUST_BACKTRACE` to print the stacktrace.
    pub use log::{debug, error, info, trace, warn};
    pub use lotto_log::init;
}

// Re-export wrapped types
pub mod base;
pub mod brokers;
pub mod cli;
pub mod collections;
pub mod engine;
pub mod owned;
pub mod sync;
pub mod sys;
