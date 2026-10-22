# Lotto

A tool to **debug** concurrent systems. Lotto helps the developer to find, reproduce and understand concurrency bugs.
Note that Lotto's primary target **is not** searching for unknown bugs, but rather debugging known bugs.

Lotto intercepts and controls the execution of multithreaded systems, exploring schedulings that are likely to trigger concurrent bugs.
The developer can tune Lotto's bug search by adding Lotto annotations and helper APIs to the system under test.
Once the target bug is triggered, Lotto is capable of replaying it with or without `gdb`.
Finally, to help the developer to understand the bug, Lotto can try to find crucial scheduling decisions that contribute to the bug being triggered (see [inflection point](doc/inflex.md)).

Lotto can be built in two main execution modes:

- **pLotto**: POSIX Lotto links to a target application and controls the scheduling of the application threads.
- **QEMU support**: Lotto runs an arm64 VM under QEMU and controls the scheduling of the virtual cores through the `qemu` and `qemu_*` modules. *(experimental)*

## Quick start

Refer to the [documentation](doc/install.md) for instructions on how to build and install Lotto.
Here we have a short explanation how to compile and run *pLotto*.
Assuming a Ubuntu/Debian, get the minimal dependencies:

	sudo apt install cmake gcc xxd


Lotto uses the Dice checkout alongside this repository by default. It must
provide `dice-roll` and the module definitions in `src/mod`. Select a different
checkout with `-DDICE_SOURCE=/path/to/dice`.

Now create a build directory and compile the default Lotto configuration

    cmake -S. -Bbuild
    cd build
    make all

With `make test` you can run the unit tests, but also try running Lotto with a real program.
For that run the following command inside the `build` directory:

    ./lotto stress ./test/integration/no_mutual_exclusion

After a few seconds you should see something like the following:

```
[lotto] round: 0/inf, pos
[lotto] round: 1/inf, pos
[lotto] round: 2/inf, pos
[lotto] round: 3/inf, pos
[lotto] round: 4/inf, pos
[lotto] round: 5/inf, pos
[lotto] round: 6/inf, pos
[lotto] round: 7/inf, pos
[lotto] round: 8/inf, pos
[lotto] round: 9/inf, pos
[lotto] round: 10/inf, pos
[2] assert failed run(): ../test/integration/no_mutual_exclusion.c:16: x == 1

/tmp/liblotto/libplotto.so(+0x1805a)[0x7ffff7f7b05a]
/tmp/liblotto/libplotto.so(__assert_fail+0x101)[0x7ffff7f7b480]
./test/integration/no_mutual_exclusion(run+0xc1)[0x55555555538a]
/tmp/liblotto/libplotto.so(+0xf998)[0x7ffff7f72998]
/lib/x86_64-linux-gnu/libc.so.6(+0x94ac3)[0x7ffff7c6cac3]
/lib/x86_64-linux-gnu/libc.so.6(+0x126a40)[0x7ffff7cfea40]
```

In this example, Lotto ran the program 10 times to be able to trigger the assertion.
With `./lotto replay` you can replay the failed execution as often as you like.
If you launch the replay inside `gdb` you must set `follow-fork-mode` to `child`.
Use `./lotto debug` to start the replay in `gdb` and have it automatically configured.

> Note that *pLotto* relies on TSAN instrumentation to capture execution events.
> **All** files in your program **must** be compiled and linked with the `-fsanitize=thread` option.
> Currently only `gcc` is supported, compiling with `clang` is known to cause problems.

## Next steps

See the [installation guide](doc/install.md) for more compilation options for *pLotto*.

## Generated Dice configuration

[`lotto.dice`](lotto.dice) is the shared configuration for the runtime, driver,
and their plugins. Core declarations live in `src/dice/`; each optional
module has a definition in `modules/<name>.dice`. CMake imports only enabled
modules and writes their imports to `<build>/dice/selected.dice`, loaded by
`lotto.dice` through `(import (lotto selected))`. The source and generated
definitions are mapped into the `lotto` namespace.

Ordering is declared in the `.dice` files. `lotto.dice` sets the default
`(after _ (dice self))`; within `lotto`, ordinary slots default to running after
`runtime` and before `threads`. Early filters override those relationships in
their slot declarations. Core dependencies live in `src/dice/core.dice`, and
optional modules declare their predecessors in `modules/<name>.dice`.
The predecessor lists preserve the existing sequencer pipeline even when an
optional module in the middle is disabled. Interceptor initialization order
is application policy in `lotto.dice`.

Ordering references to absent modules are ignored. `(require ...)` in
`lotto.dice` makes the shared core slots mandatory. Requirements validate
presence without importing definitions. CMake only selects module imports;
changing its `add_module()` order does not change handler order.

Slot numbers come from Dice. A module's CMake target supplies
`DICE_MODULE_SLOT=SLOT_LOTTO_<NAME>` and includes the generated configuration
before the public headers. Builtin handlers use direct dispatch; loadable
modules define `DICE_PLUGIN_MODULE` and register callbacks at their declared
slots. Runtime and driver use the same slot assignments and configuration ID.
Rebuild plugins whenever the configuration changes.

Each compiled target has a fixed default slot. Ordinary consumers use
`PS_SUBSCRIBE(chain, event, ...)`; a source that needs another slot uses
`PS_SUBSCRIBE_SLOT(chain, event, slot, ...)`. For example, with clock as the
target's default:

```c
PS_SUBSCRIBE(CAPTURE_EVENT, EVENT_CLOCK_READ, {
    /* Uses SLOT_LOTTO_CLOCK. */
})

PS_SUBSCRIBE(CHAIN_DICE_CONTROL, EVENT_DICE_INIT, {
    /* Initialization that follows the declared slot order. */
})
```

An explicit subscription such as
`PS_SUBSCRIBE_SLOT(CHAIN, EVENT, SLOT_OTHER, { ... })` selects another declared
slot and supplies that slot's route context for publications inside its body.
Set `DICE_MODULE_SLOT` before including the generated interface and keep it
fixed throughout the source file. Redefining it later does not update the
publication context or the dispatch/plugin mode chosen by the header.
Linked consumers use `DICE_DISPATCH_MODULE`; plugins use `DICE_PLUGIN_MODULE`,
which takes precedence if both flags are set.

An initialization subscription runs when Dice dispatches `EVENT_DICE_INIT`,
in slot order. Its constructor only registers it. All participating plugins
must be loaded before that event; a plugin loaded later cannot participate in
the initialization that already ran. This does not change Lotto's existing
startup/loader sequencing. Unlike `DICE_MODULE_INIT`, the subscription has no
once-only guard of its own or separate constructor initialization.

Modules can have multiple slots. The thread forwarding target defaults to
`SLOT_LOTTO_THREADS`, separately from `SLOT_LOTTO_RUNTIME`.
`FORWARD_CAPTURE_TO_INGRESS(suffix, event)` uses its target's default slot.
Modules such as `uafcheck` and `leakcheck` compile shared sources for distinct
memory-manager slots, with a fixed default for each target.

The repeated `LOTTO_SUBSCRIBE`, state, and flag macros retain their existing
`1000 * DICE_MODULE_SLOT + __COUNTER__` callback slots. This preserves their
ordering while the extension-slot redesign remains a separate change.
Public Lotto event and chain IDs are pinned in `src/dice/events.dice` to retain
the guest interface values. Numeric header definitions remain available to
standalone legacy test fixtures and guest programs.

Use `-DLOTTO_CHECK_ROUTES=ON` to validate publications against the declarations.
For example:

```sh
cmake -S . -B build -DDICE_SOURCE=../dice \
  -DCMAKE_BUILD_TYPE=Release -DLOTTO_LTO=ON -DLOTTO_CHECK_ROUTES=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

The `dice-record-replay` test records and replays a pthread program with a
loaded probe. It checks repeated registration callback order and event
delivery. It is enabled when the blocking, clock, and join modules are enabled.
