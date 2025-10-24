# the Rust plugin

## Crates

- `lotto`: the bindings of the lotto interface
  - `lotto_sys` and `lotto_macros` are re-exported from `lotto`
- `plugin`: imports everything for building as .so file

Other crates are the actual implementation of handlers, commands, etc.
