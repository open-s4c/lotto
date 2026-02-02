//! # Unit test linker hack
//!
//! `lotto` bindings (including `lotto_sys`) by default are not linked
//! with Lotto libraries. This means unit tests are basically
//! unusable.
//!
//! This library is a hack that causes any dependents to be linked
//! with a special static library that contains all lotto C symbols
//! for testing purposes.
//!
//! ## Usage
//!
//! 1. Add `lotto_link` to `dev-dependencies` in the Cargo.toml
//!
//! 2. Reference `lotto_link` somewhere inside the `test` target
//!
//!    ```ignore
//!    #[cfg(test)]
//!    mod tests {
//!        extern crate lotto_link;
//!        // Other normal code follows
//!    }
//!    ```
//!
//! Voilà! The crate should be cargo-testable now.
