# Lotto Rust

Lotto has comprehensive support for Rust extensions through
the plugin system and the `lotto` crate.  This README should give you
some basic ideas on how to write Lotto-flavored Rust, safely.

For API documentation, use `cargo doc --open`.

## Prerequisites

- Install Rust (1.74 or newer)
- Configure lotto with the option `-DLOTTO_RUST=ON`

## Add a new Rust handler

Currently, Rust static libraries have the limitation that only **one** Rust static library may be linked into
a foreign language executable, otherwise you will likely get linker errors due to duplicated symbols from the
Rust standard library. To work around this, we only link one static library - the `rusty_engine` - into our engine.
If you wish to add a new component in Rust to the engine, then please create a new `crate`
(e.g. `cargo new --lib handlers/my_awesome_handler` ) and have the `rusty_engine` depend on it
(`cargo add --path handlers/my_awesome_handler`). The `rusty_engine` basically acts as a wrapper library for all Rust 
code that should be linked into lotto engine.

## Calling Lotto c-functions

We use [bindgen] to automatically generate Rust bindings for C types and functions. The required C headers are 
added on an as-needed basis to `lotto_sys/wrapper.h`.
The `build.rs` script in that folder will use `bindgen` to generated Rust code for the bindings. The script can
be modified, to e.g. finetune how a type is translated, or to e.g. block a certain header that is 
transitively included and causing problems.

If you need to debug raw bindings or simply take a look - they will be generated to 
`$CMAKE_BUILD_DIR/cargo/build/x86_64-unknown-linux-gnu/RelWithDbg/build/lotto_sys-<some_hash>/out/bindings.rs`.
(If you are cross-compiling (e.g. to aarch64), the target specific component of the path will be different).
This generated raw binding is directly included into the [`raw.rs`](../../rust/lotto_sys/src/raw.rs) module of
`lotto_sys`. These bindings are just a direct 1:1 translation of the C definitions. For some commonly used
types we will add handwritten more ergonomic Rust variants and provide a convenient conversion functions via the Rust
`From`, `TryFrom`, `Into` and `TryInto` traits.

## Exposing Rust to C

In your Rust code you can define functions with `C-ABI: 

```rust
#[no_mangle]
pub extern "C" fn my_rust_function() {
    // ...
}
```

In simple cases you can just write the corresponding C function definition yourself to call the function. If it gets 
more complicated, e.g. you need to access some types defined on the Rust side, then we could generate c-bindings by
using [`cbindgen`](https://github.com/mozilla/cbindgen).


## Logging

Please always use `lotto::log`.  It re-exports the standard log macros
from the [`log`] crate, namely `debug!`, `info!`, `warn!`, `error!`.
Log messages are redirected to the Lotto logging system as
expected.

You can control the verbosity with standard Lotto CLI flags and
environment variable `LOTTO_LOG_LEVEL`.

One particular case is `trace!`, which is not part of the Lotto
logging system, and its behavior is controlled by `LOTTO_RUST_TRACE`.

The format of `LOTTO_RUST_TRACE` inputs:

```
<input>   ::= | <element> (, <element>)* ,?
<element> ::= <type>            Apply to all log targets
            | <regex>=<type>    Apply to log targets matching <regex>
<type>    ::= discard           Discard all messages
            | debug             Redirect to the debug level
            | stderr            Redirect to stderr
            | <path>            Redirect trace messages to a file
```

- If a log target matches multiple `<element>`s, only the first takes effect.
- If a log target matches none of them, the messages will be discarded.

Some examples:

```
LOTTO_RUST_TRACE=discard
LOTTO_RUST_TRACE=handler_.*=stderr,./logfile
LOTTO_RUST_TRACE=handler1=./log1,handler2=./log2,stderr
```

### Help - I'm getting warnings that a type is "not FFI-safe"

Please note that not all C types are FFI-safe, so there are not all types can be soundly passed to Rust (or even to 
code compiled with a different C compiler). The Rust compiler will however issue warnings.
As an example 128-bit integers currently do not have a stable cross-language ABI (the required alignment differs), 
even when using the LLVM backend for both C and Rust.


[bindgen]: https://rust-lang.github.io/rust-bindgen/
[`log`]: https://docs.rs/log/latest/log/
