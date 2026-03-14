# Lotto Initialization

## Goal

Lotto startup should follow one invariant:

1. Constructors only wire the system together.
2. `EVENT_LOTTO_REGISTER` performs registration work.
3. `EVENT_LOTTO_INIT` performs initialization work.
4. When `EVENT_LOTTO_INIT` returns, Lotto is fully initialized in that
   process.

In short:

`constructors -> EVENT_DICE_READY -> EVENT_LOTTO_REGISTER -> EVENT_LOTTO_INIT`

This document describes the intended model, how builtin and plugin modules fit
into it, and the places where the current tree still deviates from it.

## Scope

Lotto runs in more than one process image:

- The CLI process loads `lotto-driver.so` and the enabled driver plugins.
- The instrumented target process loads `liblotto.so` and the enabled runtime
  plugins through `LD_PRELOAD`.

Each process executes its own initialization sequence. The phase model applies
independently to each one.

## Phase 1: Dice Subscription

Phase 1 is load-time wiring.

At this stage, constructors may do only the minimum needed to make later phases
possible:

- advertise pubsub chains and event types
- subscribe handlers to Dice or Lotto pubsub chains
- perform other small, order-only setup that must exist before the first event
  publication

Typical helpers involved here are:

- `PS_SUBSCRIBE(...)`
- `LOTTO_SUBSCRIBE(...)`
- `LOTTO_SUBSCRIBE_CONTROL(...)`
- `PS_ADVERTISE_CHAIN(...)`
- `LOTTO_ADVERTISE_TYPE(...)`

The important rule is that semantic registration must not happen here. In
particular, constructors should not allocate user-visible Lotto state, register
categories, register CLI commands, or run module-specific init logic.

## Phase 2: Lotto Registration

Once Dice pubsub becomes ready, Lotto enters the registration phase.

The intended mechanism is:

- `runtime/module.c` subscribes to `EVENT_DICE_READY`
- `driver/module.c` subscribes to `EVENT_DICE_READY`
- those subscribers publish `EVENT_LOTTO_REGISTER`

All Lotto modules then react to `EVENT_LOTTO_REGISTER` and perform registration
work there, for example:

- category registration via `new_category(...)`
- flag registration via `new_flag(...)`
- state registration via `statemgr_register(...)`
- driver-side table setup that must exist before command init

Registration is allowed to build registries and tables, but it should not
assume that later init work has already happened.

## Phase 3: Lotto Init

After registration finishes, the same `EVENT_DICE_READY` subscribers publish
`EVENT_LOTTO_INIT`.

All modules that require post-registration setup should do it here. Typical
examples are:

- driver command registration with `subcmd_register(...)`
- module-specific init that depends on flags, categories, or other registries
  already existing

The intended meaning of the phase boundary is:

- before `EVENT_LOTTO_INIT`: registries may exist, but Lotto is not yet ready
- after `EVENT_LOTTO_INIT`: Lotto is ready for normal operation in that process

## Builtin And Plugin Modules

Builtin modules and plugin modules should follow the same phase contract.

### Builtin modules

Builtin modules are linked into `lotto-driver.so` or `liblotto.so`. Their
constructors run when the shared object is loaded, so they can participate in
Phase 1 by advertising types and installing subscriptions.

### Plugin modules

Plugin modules are separate shared objects. In practice:

- driver plugins are preloaded into the CLI process before `driver_main()`
- runtime plugins are preloaded into the child process before the target
  program's `main()`

That means plugin constructors also run before the first Dice publication, so
plugins can use the same Phase 1 wiring model as builtin modules. After that,
plugins should behave exactly like builtins:

- subscribe during Phase 1
- register during `EVENT_LOTTO_REGISTER`
- initialize during `EVENT_LOTTO_INIT`

## Pubsub And Ordering Notes

### Dice readiness

Dice publishes `EVENT_DICE_INIT` and then `EVENT_DICE_READY` from
`ps_initd_()`. Lotto should treat `EVENT_DICE_READY` as the point where the
Lotto phase machine starts.

### Subscription order

`LOTTO_SUBSCRIBE_CONTROL(...)` ultimately subscribes with a slot derived from
`DICE_MODULE_SLOT` and a per-translation-unit counter:

- `DICE_MODULE_SLOT` orders modules at a coarse level
- the counter orders multiple subscriptions within the same compilation unit

This is enough for deterministic handler order when order matters.

### Same-slot state registration

`statemgr` explicitly binds multiple registrations that share the same
`DICE_MODULE_SLOT`. That removes dependence on constructor order for config
state such as PRNG, sequencer, and category state.

### Constructor priorities

The current tree uses several constructor classes:

- Dice constructors via `DICE_CTOR`
- Lotto helper constructors via `LOTTO_CONSTRUCTOR` (priority `101`)
- `LOTTO_MODULE_INIT_` currently as constructor priority `1002`

Constructor priorities should be treated as a wiring detail, not as the main
initialization mechanism. The semantic phase ordering must come from pubsub
events, not from constructor timing.

## Current Deviations

The intended strategy is not yet fully enforced everywhere.

### 1. `LOTTO_MODULE_INIT_` bypasses `EVENT_DICE_READY`

`include/lotto/core/module.h` currently implements `LOTTO_MODULE_INIT_` as a
constructor with priority `1002`, while the intended `EVENT_DICE_READY`
subscription is left in the disabled `#if 0` branch.

Effect:

- `runtime/module.c` and `driver/module.c` do not currently wait for
  `EVENT_DICE_READY`
- the Lotto phase sequence is started directly from a constructor instead of
  from Dice readiness

This is the main mismatch with the intended design.

### 2. Runtime-local init still happens after `EVENT_LOTTO_INIT`

`src/runtime/module.c` currently does:

1. publish `EVENT_LOTTO_REGISTER`
2. publish `EVENT_LOTTO_INIT`
3. call `runtime_init()`

Effect:

- runtime-local initialization is outside Phase 3
- after `EVENT_LOTTO_INIT` returns, the runtime is not yet fully initialized
- `EVENT_LOTTO_INIT` subscribers cannot assume `runtime_init()` has completed

If the project definition is that Lotto is fully initialized once
`EVENT_LOTTO_INIT` completes, this ordering must change.

### 3. Some plugins still do semantic work in `DICE_MODULE_INIT`

Most plugin `module.c` files use `DICE_MODULE_INIT()` only as a marker, which
is harmless. But `modules/debug/src/module.c` does real work in
`DICE_MODULE_INIT`, calling `debug_dump_assets(...)` at module-init time.

Effect:

- the work bypasses `EVENT_LOTTO_REGISTER`
- the work bypasses `EVENT_LOTTO_INIT`
- the module does semantic initialization during Phase 1

That should be moved behind the Lotto phase model or explicitly documented as a
deliberate exception.

## Practical Rule For New Code

For Lotto modules, use this checklist:

1. Constructors may advertise and subscribe.
2. `EVENT_LOTTO_REGISTER` may populate registries and tables.
3. `EVENT_LOTTO_INIT` may perform post-registration init.
4. Do not put semantic module work in `DICE_MODULE_INIT` or ad-hoc
   constructors.

## Recommended Cleanup

To make the code match the strategy:

1. Restore `LOTTO_MODULE_INIT_` to subscribe to `EVENT_DICE_READY`.
2. Move runtime-local init into the Lotto phase model, or redefine the phase
   boundary explicitly if `runtime_init()` must stay outside it.
3. Audit plugin `DICE_MODULE_INIT({...})` bodies and move semantic work to
   `EVENT_LOTTO_REGISTER` or `EVENT_LOTTO_INIT`.
