# Current Handoff

This file is a practical handoff for a fresh coding session on Lotto.
It describes the current working model, not the historical migration.

Before reasoning about semantic event flow, also read
[doc/ingress.md](/home/diogo/Workspaces/lotto/doc/ingress.md). That file is
the focused reference for ingress ownership, invariants, and publication
patterns.

Also read the relevant files under [doc/](/home/diogo/Workspaces/lotto/doc).
Do not treat the handoff in `AGENTS.md` as sufficient when there is detailed
design or usage documentation in `doc/`.

## Branch / Scope

- Branch often used for this work: `transition-to-dice-events`
- Current working branch may differ; check `git branch --show-current`
- Non-QEMU runtime/engine path is the main model below
- QEMU is separate and should not be inferred from the non-QEMU path

## Current Runtime Event Path

The active non-QEMU path is:

`INTERCEPT_* -> CAPTURE_* -> INGRESS_* -> mediator -> engine -> sequencer -> plan -> switcher`

Where:

- `INTERCEPT_*` is raw interception
- `CAPTURE_*` is Dice `self` plus task/thread metadata
- `INGRESS_*` is semantic Lotto ingress

Active ingress chains:

- `CHAIN_INGRESS_EVENT`
- `CHAIN_INGRESS_BEFORE`
- `CHAIN_INGRESS_AFTER`

Important invariants:

- ingress pubsub event type is the semantic event id
- `capture_point.src_type` must equal that same semantic event id
- [src/runtime/ingress.c](/home/diogo/Workspaces/lotto/src/runtime/ingress.c)
  asserts `cp->src_type == type`
- sequencer chains publish semantic event ids directly
- sequencer payload is `capture_point *`
- sequencer decision state is embedded in the `capture_point`

The old synthetic sequencer event ids are gone:

- `EVENT_SEQUENCER_CAPTURE`
- `EVENT_SEQUENCER_RESUME`

The old runtime-ingress compatibility framing is also not the active non-QEMU
model anymore.

## Structural Rule

The important distinction is:

- `_EVENT`
- `_BEFORE/_AFTER`

`_EVENT` is a single Lotto semantic operation. `_BEFORE/_AFTER` brackets an
operation span.

Examples:

- `mutex`, `rwlock`, `evec`, `join` semantic operations are Lotto-level events
- true external bracketing operations go through `_BEFORE/_AFTER`

`CALL` should be treated as legacy baggage when reading older docs or code
comments.

## Startup Model

Generic Lotto startup now lives in
[src/engine/module.c](/home/diogo/Workspaces/lotto/src/engine/module.c).

That file is already part of both:

- `lotto-runtime-engine.o`
- `lotto-driver-engine.o`

Current startup owner behavior:

- forces Dice initialization with a dummy control publication on
  `EVENT_LOTTO_IGNITE`
- closes the late-subscription barrier
- starts `EVENT_LOTTO_REGISTER`
- starts `EVENT_LOTTO_INIT`

Useful consequence:

- the dummy publication is the proof boundary that Dice `ps_init()` and the
  loader `dlopen` pass have completed
- Lotto does not rely on `EVENT_DICE_READY` for startup ordering

The active rule is:

- raw `ps_subscribe(...)` must be complete before Lotto registration starts

Runtime-side details:

- [src/runtime/module.c](/home/diogo/Workspaces/lotto/src/runtime/module.c)
  is now just an empty `DICE_MODULE_INIT()`
- [src/driver/module.c](/home/diogo/Workspaces/lotto/src/driver/module.c)
  no longer owns generic Lotto startup

## Driver / Runtime Split

Two mutually exclusive Dice instances:

- runtime process -> `liblotto.so`
- driver/CLI process -> `liblotto-driver.so`

Plugin families are also mutually exclusive:

- runtime process -> `lotto-runtime-*.so`
- driver/CLI process -> `lotto-driver-*.so`

Do not assume both plugin families belong in one process.

### QEMU Artifact Rule

For QEMU, the artifacts passed to `qemu-system-*` via `-plugin` must come from
the Lotto runtime artifact family only.

There are two cases:

- base QEMU support compiled into the main runtime library
- additional QEMU feature modules compiled as `lotto-runtime-qemu*.so`

Precise rule:

- if `modules/qemu` is `BUILTIN`, the base QEMU plugin passed to QEMU is
  `liblotto-runtime.so`
- any additional enabled QEMU feature modules are passed as extra QEMU plugins:
  - `lotto-runtime-qemu_gdb.so`
  - `lotto-runtime-qemu_profile.so`
  - `lotto-runtime-qemu_snapshot.so`
- `stress -Q` / `lotto qemu` must therefore:
  - pass the base runtime plugin that contains QEMU support first
  - then pass any enabled `lotto-runtime-qemu*.so` feature modules as
    additional `-plugin` arguments

Do not introduce or preserve a separate `libplugin-*` artifact family for QEMU
features.

QEMU preload note:

- do not assume that `LD_PRELOAD=liblotto-runtime.so` plus
  `qemu-system-* -plugin liblotto-runtime.so` creates two independent Lotto
  runtime instances in this tree
- current local working assumption is that QEMU's `dlopen(...)` should reuse
  the already preloaded `liblotto-runtime.so` handle
- do not force `--no-preload` for QEMU based on a duplicate-runtime assumption
  without stronger evidence

Early QEMU submodule configuration rule:

- if a `qemu*` submodule needs configuration at QEMU plugin-init time, do not
  forward it via environment variables
- forward it as arguments on the base QEMU plugin
- base `qemu` then republishes that data on `CHAIN_QEMU_CONTROL` /
  `EVENT_QEMU_INIT`
- submodules must derive their own config/state from that control event

Reason:

- normal Lotto `CONFIG` replay is too late for QEMU callback registration
  performed during plugin initialization

## Dice Self Guard (Important)

The Dice `self` core module already implements the recursion/reentrancy guard
for intercept->capture republishing.

Reference behavior:

- implementation: `deps/dice/src/mod/self.c`
- API: `deps/dice/include/dice/self.h` (`self_guard_get`)
- test: `deps/dice/test/self/self_guard_test.c`

How it works:

- `INTERCEPT_BEFORE`: increments guard; only republishes to `CAPTURE_BEFORE`
  when entering at guard 0
- `INTERCEPT_AFTER`: republishes to `CAPTURE_AFTER` only at matching outer
  level; then decrements guard
- nested intercepted publications are intentionally blocked from recursive
  capture republish when already inside serving state

Rule for Lotto code:

- do not add new ad-hoc recursion guards in Lotto intercept paths just to
  mirror Dice `self` behavior (for example allocator-local depth guards)
- when guard-sensitive logic is needed, read the Dice guard state via
  `self_guard_get(md)` and align behavior with Dice `self` semantics instead of
  introducing a parallel guard model

## Replay / Envvar Rule

`exec_info_replay_envvars()` must replay envvars individually and must
`unsetenv(...)` when the recorded value is empty.

This matters especially for `DICE_PLUGIN_MODULES`:

- embedded-runtime traces can legitimately record `DICE_PLUGIN_MODULES=""`
- replay must then unset it
- otherwise driver-side plugin lists can leak into the runtime Dice instance

## Runtime / Engine Components

### Ingress

[src/runtime/ingress.c](/home/diogo/Workspaces/lotto/src/runtime/ingress.c)
is the direct handoff:

- semantic ingress arrives
- `mediator_capture`
- immediate resume for non-blocking cases
- `mediator_return` on `_AFTER` for blocking cases

### Mediator

Mediator is the runtime-side protocol executor:

- keep per-task mediator state
- call the engine
- execute the returned plan through the switcher

It is intentionally thinner than older versions. Avoid re-growing it into a
general task-local substrate.

### Engine

The engine checks capture/return/resume invariants and delegates scheduling
choice composition to the sequencer.

### Sequencer

The sequencer:

- consumes semantic `capture_point`s
- computes candidate tasks
- publishes to handler chains
- resolves the next task
- forms the plan
- records replay/trace state

Active handler boundary:

- `CHAIN_SEQUENCER_CAPTURE`
- `CHAIN_SEQUENCER_RESUME`

with semantic event ids and `capture_point *` payloads.

### Switcher

Switcher is the narrow primitive for:

- wake
- yield
- wait/block transitions

## Current State Of Context / Categories

Still true:

- `capture_point` is the active semantic carrier
- many handlers now consume `capture_point` directly

Still not fully gone:

- `context_t`
- `category_t`

When working in the tree, prefer the semantic event id plus `capture_point`
model over old category-centric reasoning.

## Module / Event Ownership

- runtime/core semantic ids live under runtime headers
- module-owned semantic ids live in each module’s own `events.h`

Do not reintroduce a shared umbrella header for module events. Include the
specific module `events.h` that a file actually needs.

## Recent Practical Fixes Worth Remembering

- `join` takeover path must publish `EVENT_TASK_JOIN` with
  `cp.src_type = EVENT_TASK_JOIN`
- semantic ingress forwarders must preserve the invariant
  `cp->src_type == published type`
- `mutex` and `evec` flattened their interceptor/subscriber flow so semantic
  `capture_point`s are created directly and forwarded explicitly
- sequencer now publishes semantic event ids directly on its chains
- `capture_point` now carries embedded sequencer decision state
- Rusty statemgr module slot is generated in `lotto_sys/build.rs` from the
  configured `rusty.h`, instead of relying on bindgen to expose the slot macro

## Builds Commonly Used

- `build`
  - main non-Rust local build
- `build-clang`
  - clang build
- `build-rust`
  - Rust-enabled build
- `build-off`
  - often used for startup/replay debugging with `-DLOTTO_MODULE_rusty=OFF`

Common commands:

- `make -C build -j4`
- `make -C build test -j4`
- `make -C build tikl-test -j4`
- `make -C build-clang tikl-test -j4`
- `make -C build-rust -j4`

Build validation rule:

- only a build of the whole project counts as "build OK"
- partial target builds are useful for iteration, but they do not count as a
  final build-success claim

## Test Notes

- `0006-racy-message-passing.c` may still be intentionally disabled depending
  on branch state; do not assume it is part of the stable passing set
- filtering tests now use `scripts/lotto-events` instead of hardcoded event ids
- Rust-enabled builds may regenerate Cargo artifacts and can expose bindgen
  issues that do not appear in plain C builds

## Main Documentation Pointers

- [doc/design.md](/home/diogo/Workspaces/lotto/doc/design.md)
  - canonical high-level design doc
- [claims.md](/home/diogo/Workspaces/lotto/claims.md)
  - current startup proof checklist / notes

## Good First Checks In A New Session

1. Check current branch and `git status`
2. Identify which build dir matches the issue:
   - `build`
   - `build-clang`
   - `build-rust`
   - `build-off`
3. Prefer semantic event flow reasoning:
   - published type
   - `cp->src_type`
   - ingress chain
   - sequencer chain
4. If startup/replay behaves strangely, inspect:
   - `src/engine/module.c`
   - replayed envvars
   - `DICE_PLUGIN_MODULES`

## Formatting

- Do not run raw `clang-format` directly on broad file globs from the repo
  root. Use [scripts/clang-format.sh](/home/diogo/Workspaces/lotto/scripts/clang-format.sh)
  to format source files and avoid touching unsupported files.
- Do not run raw `clang-format` on CMake files. Use
  [scripts/cmake-format.sh](/home/diogo/Workspaces/lotto/scripts/cmake-format.sh)
  for CMake formatting.
- Before finishing a turn, format modified source files with the helper
  scripts above rather than ad hoc formatter invocations.

## Header Layout Audit

This section is the step-1 audit for the header-layout cleanup.

Scope:
- Included: in-tree Lotto headers under `include/`, `src/include/`, `modules/*/include/`, module-local headers in source trees, and test headers.
- Excluded: vendored headers under `deps/` and generated headers under `build*/`.

Classification:
- `public installed API`: headers that should exist in an installed-tree public surface.
- `private shared inside Lotto`: headers that may be shared across Lotto subtrees, but should not be part of the installed public API.
- `local-only`: headers that should stay next to one implementation area, test, or build step.

Installed-tree requirement:
- After installation, it must still be possible to build Lotto plugin modules
  against the installed headers and libraries.
- That means the installed public header surface must cover both:
  - SUT-facing annotation/helper APIs
  - module-author-facing APIs needed to implement out-of-tree plugins
- Any header required to write a plugin module cannot remain effectively
  private under `src/include/...` or rely on non-installed internal headers.

Target installed namespace model:
- `lotto/*.h`: SUT-facing annotation helpers, for example `lotto/yield.h`.
- `lotto/base/*.h`: public base abstractions that plugin authors may need.
- `lotto/sys/*.h`: public system-level abstractions that plugin authors may need.
- `lotto/util/*.h`: public utility headers intentionally exposed to plugin authors.
- `lotto/engine/*.h`: engine-facing module APIs.
- `lotto/driver/*.h`: driver-facing module APIs.
- `lotto/runtime/*.h`: runtime-facing module APIs.
- `lotto/modules/<name>/*.h`: module-to-module public interfaces.

Build-system policy:
- Headers under `src/include/...` are private Lotto implementation headers
  unless explicitly promoted into the installed public surface above.
- Module targets should not use broad access to private Lotto headers via
  `include_directories(${PROJECT_SOURCE_DIR}/src/include)` or equivalent.
- If a module needs a header from `src/include/...`, that is a signal to either
  promote that header into the installed public surface, or refactor the module
  to stop depending on private internals.

### Public Installed API

#### Existing public-tree headers

- `include/lotto/check.h`
- `include/lotto/cli/preload.h`
- `include/lotto/driver/args.h`
- `include/lotto/driver/exec.h`
- `include/lotto/driver/exec_info.h`
- `include/lotto/driver/files.h`
- `include/lotto/driver/flagmgr.h`
- `include/lotto/driver/flags/memmgr.h`
- `include/lotto/driver/flags/prng.h`
- `include/lotto/driver/flags/sequencer.h`
- `include/lotto/driver/record.h`
- `include/lotto/driver/replay.h`
- `include/lotto/driver/subcmd.h`
- `include/lotto/driver/trace.h`
- `include/lotto/driver/trace_prepare.h`
- `include/lotto/driver/utils.h`

#### Module-facing public headers

- `modules/blocking/include/lotto/modules/blocking/blocking.h`
- `modules/deadlock/include/lotto/modules/deadlock/events.h`
- `modules/deadlock/include/lotto/rsrc_deadlock.h`
- `modules/enforce/include/lotto/modules/enforce/events.h`
- `modules/evec/include/lotto/evec.h`
- `modules/explore/include/lotto/modules/explore/explore.h`
- `modules/fork/include/lotto/fork.h`
- `modules/ichpt/include/lotto/modules/ichpt/ichpt.h`
- `modules/mutex/include/lotto/modules/mutex/mutex.h`
- `modules/mutex/include/lotto/mutex.h`
- `modules/order/include/lotto/order.h`
- `modules/poll/include/lotto/modules/poll/poll.h`
- `modules/priority/include/lotto/priority.h`
- `modules/qemu/include/lotto/qemu.h` *(experimental)*
- `modules/region_preemption/include/lotto/region_atomic.h`
- `modules/region_preemption/include/lotto/region_preemption.h`
- `modules/rusty/include/lotto/await.h`
- `modules/rusty/include/lotto/await_while.h`
- `modules/task_velocity/include/lotto/velocity.h`
- `modules/terminate/include/lotto/modules/terminate/flags.h`
- `modules/timeout/include/lotto/modules/timeout/timeout.h`
- `modules/timeout/include/lotto/modules/timeout/events.h`
- `modules/uafcheck/include/lotto/modules/uafcheck/uafcheck.h`
- `modules/user_mempool/include/lotto/modules/mempool/mempool.h`
- `modules/yield/include/lotto/modules/yield/events.h`
- `modules/yield/include/lotto/yield.h`

#### `src/include` headers that should become installable

Base: `src/include/lotto/base/` — address_bdd, arg, bag, callrec, cappt,
category, clk, context, envvar, flag, flags, map, marshable, memory_map,
plan, reason, record, record_granularity, slot, stable_address, strategy,
string, string_hash_table, task_id, tidbag, tidmap, tidset, trace,
trace_chunked, trace_file, trace_flat, value, vec.

Core: `src/include/lotto/core/` — driver/events, engine/events, events,
runtime/events.

Engine: `src/include/lotto/engine/` — catmgr, clock, engine, prng, pubsub,
recorder, sequencer, statemgr.

Runtime: `src/include/lotto/runtime/` — ingress, runtime.

Sys: `src/include/lotto/sys/` — abort, assert, ensure, fcntl, logger,
memmgr_runtime, memmgr_user, memory, mempool, poll, pthread, random, sched,
signal, socket, spawn, stdio, stdlib, stream, stream_chunked,
stream_chunked_file, stream_ffile, stream_file, string, time, unistd, wait.

Util: `src/include/lotto/util/` — casts, contract, iterator, macros, once,
prototypes, strings, unused.

### Private Shared Inside Lotto

Module-private state headers (`modules/*/include/lotto/modules/*/state.h`)
are private but live under a public-looking namespace — these should not be
installed.

`src/include` internal-only headers include: `base/trace_impl.h`,
`cli/cli_stress.h`, `cli/constraints.h`, `driver/main.h`,
`engine/dispatcher.h`, `engine/qemu_plugin.h`, `engine/state.h`,
`engine/util/qemu_gdb.h`, `engine/util/symlist.h`, `runtime/mediator.h`,
`runtime/switcher.h`, `sys/logger_block.h`, `sys/memmgr_impl.h`,
`sys/modules.h`, `sys/now.h`, `sys/real.h`, `sys/signatures/*`,
`sys/stream_chunked_impl.h`, `sys/stream_impl.h`, `util/redirect.h`.

### Local-Only

Build inputs and implementation-local: `include/lotto/unsafe/_sys.h`,
`modules/debug/src/debug.h`, `modules/fork/include/state.h`,
`modules/priority/include/category.h`, `modules/priority/include/state.h`,
`modules/rinflex/include/constraint_satisfaction.h`,
`modules/rusty/lotto_sys/wrapper.h`, `src/runtime/interceptor_internal.h`,
`src/runtime/sighandler.h`.

Test-only: `test/integration/quack.h`, `test/integration/safe_stack.h`,
`test/integration/safe_stack_sparse.h`,
`test/racket/engine/mock/engine_mock.h`,
`test/tsan/llvm_15_0_4/include/tsan_interface_atomic.h`,
`test/unit/base/mock/memory_mock.h`.

### Immediate Cleanup Targets

- `src/include/lotto/base/*` and other `src/include` subtrees are public
  candidates currently living in the private tree.
- `modules/*/include/lotto/modules/*/state.h` headers are private but live
  under a public-looking namespace.
- `modules/user_mempool/include/lotto/modules/mempool/mempool.h` is a public
  candidate that currently includes the private header
  `src/include/lotto/sys/memmgr_impl.h`.
- `include/lotto/cli/preload.h` is public by usage, but its namespace suggests
  CLI internals rather than a stable driver/module extension API.

## Startup Claims
- Event payload types should be declared as plain `struct ...`, not `typedef ..._t`.

Status legend:
- `Holds`: checked in current tree.
- `Partial`: mostly true, but with a known caveat.
- `Unknown`: not proved yet.
- `Superseded`: the old shape no longer exists; the replacement claim is listed.

Focused verification status:
- `cmake --build build-off --target tikl-0001-mutex-overwrite-var -j4` passes with `LOTTO_RUST=OFF`.
- Full `tikl-test` has not been rerun after the current startup refactor.

### Core startup claims

1. All active Dice constructors use `DICE_XTOR_PRIO`, and the old loader constructor is inactive.
   Status: `Holds`
   Evidence: [CMakeLists.txt](/home/diogo/Workspaces/lotto/CMakeLists.txt), [deps/dice/include/dice/compiler.h](/home/diogo/Workspaces/lotto/deps/dice/include/dice/compiler.h), [deps/dice/src/dice/loader.c](/home/diogo/Workspaces/lotto/deps/dice/src/dice/loader.c)

2. All Lotto constructors in mothers/plugins use `DICE_XTOR_PRIO`, except the single generic startup constructor.
   Status: `Holds` for non-Rust, non-QEMU Lotto-owned code
   Evidence: [src/engine/module.c](/home/diogo/Workspaces/lotto/src/engine/module.c), [src/include/lotto/engine/pubsub.h](/home/diogo/Workspaces/lotto/src/include/lotto/engine/pubsub.h)
   Caveat: Rust and QEMU were explicitly excluded from this audit. External DSOs were not audited for their own constructors.

3. The mother library is already loaded by `LD_PRELOAD`, and Dice loader only `dlopen`s plugin DSOs from `DICE_PLUGIN_MODULES`.
   Status: `Partial`
   Evidence: [deps/dice/src/dice/loader.c](/home/diogo/Workspaces/lotto/deps/dice/src/dice/loader.c), [src/driver/preload.c](/home/diogo/Workspaces/lotto/src/driver/preload.c)
   Caveat: the loader does prefer `DICE_PLUGIN_MODULES`, but the runtime preload path can prepend memmgr plugins before `liblotto.so`, so “mother library is first in `LD_PRELOAD`” is not globally true today.

4. `LOTTO_MODULE_INIT` is a plain constructor, not an `EVENT_DICE_READY` callback.
   Status: `Superseded`
   Replacement: generic startup now lives in the plain constructor `lotto_startup_()` in [src/engine/module.c](/home/diogo/Workspaces/lotto/src/engine/module.c). The old `include/lotto/core/module.h` wrapper was removed.

5. The first thing generic Lotto startup does is a dummy `PS_PUBLISH(...)`.
   Status: `Holds`
   Evidence: [src/engine/module.c](/home/diogo/Workspaces/lotto/src/engine/module.c)

6. After that dummy publish returns, Dice has completed `ps_init()`, published `EVENT_DICE_INIT`, and finished the loader `dlopen` pass.
   Status: `Holds`
   Evidence: [src/engine/module.c](/home/diogo/Workspaces/lotto/src/engine/module.c), [deps/dice/src/dice/pubsub.c](/home/diogo/Workspaces/lotto/deps/dice/src/dice/pubsub.c), [deps/dice/src/dice/loader.c](/home/diogo/Workspaces/lotto/deps/dice/src/dice/loader.c)

7. Lotto publishes `EVENT_LOTTO_REGISTER` and `EVENT_LOTTO_INIT` only after the dummy publish returns.
   Status: `Holds`
   Evidence: [src/engine/module.c](/home/diogo/Workspaces/lotto/src/engine/module.c)

8. The late-subscription barrier closes only after the dummy publish returns.
   Status: `Holds`
   Evidence: [src/engine/module.c](/home/diogo/Workspaces/lotto/src/engine/module.c)

9. Lotto registry-building work (`statemgr`, `flags`, `commands`, `dispatcher`, categories) is attached through Lotto phase callbacks, not by ad-hoc raw `ps_subscribe(...)`.
   Status: `Holds` for audited non-Rust, non-QEMU code
   Evidence: [src/include/lotto/engine/statemgr.h](/home/diogo/Workspaces/lotto/src/include/lotto/engine/statemgr.h), [src/include/lotto/engine/dispatcher.h](/home/diogo/Workspaces/lotto/src/include/lotto/engine/dispatcher.h), [src/driver/module.c](/home/diogo/Workspaces/lotto/src/driver/module.c), [modules/priority/src/category.c](/home/diogo/Workspaces/lotto/modules/priority/src/category.c)

10. No registration-phase callback performs new raw Dice subscriptions.
    Status: `Unknown`
    Evidence: a tree search found no direct `ps_subscribe(` outside pubsub plumbing in audited non-Rust, non-QEMU Lotto code, but registration callbacks themselves were not exhaustively proved transitively.

### Ordering claims

11. `LOTTO_SUBSCRIBE_ONCE` uses exactly `DICE_MODULE_SLOT`.
    Status: `Holds`
    Evidence: [src/include/lotto/engine/pubsub.h](/home/diogo/Workspaces/lotto/src/include/lotto/engine/pubsub.h), [deps/dice/include/dice/module.h](/home/diogo/Workspaces/lotto/deps/dice/include/dice/module.h)

12. `LOTTO_SUBSCRIBE` and `LOTTO_SUBSCRIBE_CONTROL` use `DICE_MODULE_SLOT * 1000 + counter`.
    Status: `Holds`
    Evidence: [src/include/lotto/engine/pubsub.h](/home/diogo/Workspaces/lotto/src/include/lotto/engine/pubsub.h)

13. Registration/init helpers inherit that same slot scheme.
    Status: `Holds`
    Evidence: [src/include/lotto/engine/pubsub.h](/home/diogo/Workspaces/lotto/src/include/lotto/engine/pubsub.h), [src/include/lotto/engine/statemgr.h](/home/diogo/Workspaces/lotto/src/include/lotto/engine/statemgr.h), [src/include/lotto/engine/dispatcher.h](/home/diogo/Workspaces/lotto/src/include/lotto/engine/dispatcher.h)

14. Driver and runtime use the same module-slot assignments for corresponding modules.
    Status: `Holds` by build-system design
    Evidence: [cmake/modules.cmake](/home/diogo/Workspaces/lotto/cmake/modules.cmake), [src/engine/CMakeLists.txt](/home/diogo/Workspaces/lotto/src/engine/CMakeLists.txt), [src/runtime/CMakeLists.txt](/home/diogo/Workspaces/lotto/src/runtime/CMakeLists.txt), [src/driver/CMakeLists.txt](/home/diogo/Workspaces/lotto/src/driver/CMakeLists.txt)

### Still assumptions or later audits

15. No publication happens before the dummy publication in generic Lotto startup.
    Status: `Unknown`
    Notes: this is the fragile Property 2 assumption.

16. Lotto does not pull in foreign DSOs with surprising constructors.
    Status: `Unknown`
    Notes: not audited beyond Lotto-owned code.

17. Linking plugins against the mother library is safe under the clarified model.
    Status: `Unknown`
    Notes: the earlier `dead canary` corruption remains unexplained. If plugin-to-mother linkage is retried, it should be validated with the current replay envvar fix in place.
