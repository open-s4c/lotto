# Unit test linker hack

Inside `#[cfg(test)]`, add `extern crate lotto_link`, to make sure the tests are linked with all the C symbols.

Note:

1. `/build/plugins/rust/liblotto_rust_testing.a` must be present; `build` is not customizable.
2. `LOTTO_RUST` must be configured.
3. You need to build once before using `cargo test`.
