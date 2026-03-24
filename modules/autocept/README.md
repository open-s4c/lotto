# Autocept

`autocept` is an interposer module that turns selected external function
calls into Lotto blocking events. For each declared symbol it publishes
an `EVENT_AUTOCEPT_CALL` capture point carrying the current thread state,
including registers, return PC, and the symbol name.

For example, an autocept declaration file can list intercepted symbols with
[`autocept.h`](include/lotto/modules/autocept.h):

```asm
#include <lotto/modules/autocept.h>

ADD_INTERCEPTOR(sched_yield)
```

Each intercepted call is then surfaced to Lotto as an `EVENT_AUTOCEPT_CALL`
blocking point while preserving the original call context needed to inspect
and replay the transition.


## Test Components

The test setup has four parts:

- `autocept_baseline_test`
  - driver that generates randomized inputs and checks final results
- `libautocept_test_mock.so`
  - `mockoto` library that provides the real fixed-arity `foo_N` symbols
- `libautocept_testing.so`
  - interposer that wraps the `foo_N` calls
- `libautocept_checker.so`
  - subscriber library preloaded together with `autocept`

The variadic cases (`foo_13` and `foo_14`) use a small manual mock layer in
[`test/variadic_mock.c`](test/variadic_mock.c) because `mockoto` does not
model `...` correctly.

The checker is intentionally not a no-op. It asserts basic event sanity
and, on x86_64, explicitly clobbers `xmm0` through `xmm7` so floating-point
preservation is actually exercised. Manual runs print stage-by-stage success
lines:

- `before OK`
- `mock   OK`
- `after  OK`
- `caller OK`

## Commands

Build:

```bash
cmake -S . -B build
cmake --build build -j4 --target autocept_baseline_test
ctest --target-dir build -R autocept
```

Run the interposed set:

```bash
env DICE_SOURCE=deps/dice DICE_BUILD=build/deps/dice/ \
  deps/dice/scripts/dice \
  -preload build/modules/autocept/test/libautocept_testing.so:build/modules/autocept/test/libautocept_checker.so \
  build/modules/autocept/test/autocept_baseline_test
```

Useful knobs:

- `AUTOCEPT_TEST_CASE=foo_N`
- `AUTOCEPT_TEST_MODE=all`
- `AUTOCEPT_TEST_ITERS=<n>`

Test Matrix:

- stack-passed integer arguments (`foo_5`, `foo_9`, `foo_11`)
- floating-point arguments and return (`foo_8`)
- variadic integer arguments (`foo_13`)
- variadic floating-point arguments and return (`foo_14`)
- by-value aggregates larger than 16 bytes (`foo_7`, `foo_10`)
- pointer outputs with `void` return (`foo_12`)

# Design Notes

This is a redesign of a previous attempt of "automatically" intercepting
functions without precisely definig their signature. Autocept works by:

- publishing `INTERCEPT_BEFORE` from a temporary stack event
- letting Dice `self` republish that as `CAPTURE_BEFORE`
- copying the full `autocept_call_event` into `SELF_TLS()` on `CAPTURE_BEFORE`
- carrying the wrapped caller return address in `ev->retpc`
- rebuilding a fresh stack event after the real call returns
- restoring the saved event from `SELF_TLS()` on `CAPTURE_AFTER`
- preserving the real return value while restoring the original pre-call event
- on x86_64, restoring `%al` before the real call so variadic floating-point
  calls keep the correct SysV vector-argument count

That removes the old single-threaded global bridge state.

The module also carries an aarch64 implementation, but the current test and
validation focus is still x86_64.
