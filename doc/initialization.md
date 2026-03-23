# Lotto Initialization

## Goal

Lotto startup should follow one invariant:

1. Constructors only wire the system together.
2. `EVENT_LOTTO_REGISTER` performs registration work.
3. `EVENT_LOTTO_INIT` performs initialization work.
4. When `EVENT_LOTTO_INIT` returns, Lotto is fully initialized in that
   process.

In short, in the current tree:

`constructors -> EVENT_LOTTO_STARTUP_SYNC -> EVENT_LOTTO_REGISTER -> EVENT_LOTTO_INIT`

Current implementation note:

- Generic Lotto startup now lives in `src/engine/module.c`.
- It begins with a dummy `PS_PUBLISH(CHAIN_CONTROL, EVENT_LOTTO_STARTUP_SYNC, ...)`.
- That dummy publication forces Dice `ps_init()`, including the `EVENT_DICE_INIT`
  loader pass that `dlopen`s configured plugins.
- Only after that publication returns does Lotto start
  `EVENT_LOTTO_REGISTER` and `EVENT_LOTTO_INIT`.
- Lotto no longer relies on `EVENT_DICE_READY` as its startup boundary.

Some later sections in this document still describe the older
`EVENT_DICE_READY` model and should be read as historical analysis rather than
as the current implementation.

## Scope

Lotto runs in more than one process image:

- The CLI process loads `lotto-driver.so` and the enabled driver plugins.
- The instrumented target process loads `liblotto-runtime.so` and the enabled runtime
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

Once the startup sync publication returns, Lotto enters the registration phase.

The current mechanism is:

- `src/engine/module.c` performs the dummy startup-sync publication
- the same constructor then publishes `EVENT_LOTTO_REGISTER`

All Lotto modules then react to `EVENT_LOTTO_REGISTER` and perform registration
work there, for example:

- category registration via `new_category(...)`
- flag registration via `new_flag(...)`
- state registration via `statemgr_register(...)`
- driver-side table setup that must exist before command init

Registration is allowed to build registries and tables, but it should not
assume that later init work has already happened.

## Phase 3: Lotto Init

After registration finishes, the same startup constructor publishes
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

Builtin modules are linked into `lotto-driver.so` or `liblotto-runtime.so`. Their
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

> **Note:** The description below reflects the originally intended design, not
> the current implementation. The current tree uses `EVENT_LOTTO_STARTUP_SYNC`
> (a dummy publication) as the startup boundary, not `EVENT_DICE_READY`. See
> the note at the top of this document and `src/engine/module.c` for the
> active model.

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

## Registration Kinds In Lotto

This section lists the registration mechanisms used by Lotto itself, excluding
the separate QEMU callback layer.

The word "registration" is overloaded in the tree. There are two large groups:

- startup registrations, which build Lotto's global registries and behavior
- ordinary runtime container inserts, which use names like `map_register(...)`
  but are not part of module initialization

### Startup registrations

These are the registration kinds that shape Lotto as a system.

#### 1. Pubsub chain advertisement

Purpose:

- register a numeric chain id with a human-readable name for pubsub

Main APIs:

- `PS_ADVERTISE_CHAIN(...)`

Examples:

- `CHAIN_LOTTO_CONTROL`
- `CHAIN_LOTTO_DEFAULT`

This is a constructor-time wiring step.

#### 2. Pubsub event-type advertisement

Purpose:

- register a numeric event type with a human-readable name

Main APIs:

- `PS_ADVERTISE_TYPE(...)`
- `LOTTO_ADVERTISE_TYPE(...)`

Examples:

- `EVENT_LOTTO_REGISTER`
- `EVENT_LOTTO_INIT`
- `EVENT_ENGINE__REGISTER_STATE_*`

This is also a constructor-time wiring step.

#### 3. Pubsub subscription registration

Purpose:

- install event handlers into Dice or Lotto pubsub chains

Main APIs:

- `PS_SUBSCRIBE(...)`
- `LOTTO_SUBSCRIBE(...)`
- `LOTTO_SUBSCRIBE_CONTROL(...)`
- `CONTRACT_SUBSCRIBE(...)`

Used for:

- Dice capture/intercept subscribers
- Lotto control-phase subscribers
- normal Lotto event subscribers
- contract-only subscribers

This is Phase 1 wiring. The handler body may later perform registration or init
when the corresponding event is published.

#### 4. Lotto phase registration

Purpose:

- trigger the semantic Lotto startup phases

Main events:

- `EVENT_LOTTO_REGISTER`
- `EVENT_LOTTO_INIT`

These are not "registries" themselves, but they are the control points through
which the other semantic registrations are supposed to happen.

#### 5. State-manager registration

Purpose:

- register marshaled state objects into the engine state manager

Main API:

- `statemgr_register(...)`

Main helper macros:

- `REGISTER_PRINTABLE_STATE(...)`
- `REGISTER_CONFIG(...)`
- `REGISTER_CONFIG_NONSTATIC(...)`
- `REGISTER_STATIC_STATE(...)`
- `REGISTER_STATE(...)`
- `REGISTER_PERSISTENT(...)`
- `REGISTER_EPHEMERAL(...)`
- `STATEMGR_REGISTER(...)`

State kinds:

- `START`
- `CONFIG`
- `PERSISTENT`
- `FINAL`
- `EPHEMERAL`

Most state registration is triggered from `EVENT_ENGINE__REGISTER_STATE_*`,
which is itself fanned out from `EVENT_LOTTO_REGISTER`.

Rust support:

- Rust brokers also call the same `statemgr_register(...)` interface through
  FFI.

#### 6. Category registration

Purpose:

- allocate dynamic category ids beyond the built-in category range

Main API:

- `new_category(...)`

Typical use:

- modules register their custom categories during `EVENT_LOTTO_REGISTER`

Rust support:

- Rust code uses the same category registry through `new_category(...)` FFI.
- Rust registration is split by phase as well:
  `lotto_rust_register()` performs category and other registration work, and
  `lotto_rust_init()` performs post-registration init.

#### 7. CLI flag registration

Purpose:

- register command-line flags and their metadata into the global flag registry

Main API:

- `new_flag(...)`

Main helper macros:

- `DECLARE_COMMAND_FLAG(...)`
- `DECLARE_FLAG_*`
- `NEW_*FLAG(...)`

Typical timing:

- done from `EVENT_LOTTO_REGISTER`

Rust support:

- Rust CLI code can also call `new_flag(...)` through FFI.

#### 8. CLI subcommand registration

Purpose:

- register driver commands into the CLI subcommand table

Main API:

- `subcmd_register(...)`

Typical timing:

- done from `EVENT_LOTTO_INIT`

Examples:

- `record`
- `replay`
- `stress`
- `run`
- plugin-provided commands such as `debug`, `trace`, `explore`, `inflex`

Rust support:

- Rust CLI macros eventually call `subcmd_register(...)` through FFI.
- On the driver side, Rust command registration happens from
  `lotto_rust_init()`, after `lotto_rust_register()` has already handled
  categories, flags, and other registration work.

#### 9. Engine dispatcher handler registration

Purpose:

- install engine event handlers into ordered dispatcher slots

Main API:

- `dispatcher_register(...)`

Main helper macros:

- `REGISTER_HANDLER(...)`
- `REGISTER_HANDLER_EXTERNAL(...)`

This is currently constructor-based, not `EVENT_LOTTO_REGISTER`-based. It is a
registration mechanism, but not yet aligned with the phase strategy.

Rust support:

- Rust handler code also registers into the same dispatcher through FFI.

#### 10. Module registry population

Purpose:

- discover available driver/runtime modules and populate the module registry

Main APIs:

- `lotto_module_scan(...)`
- `lotto_module_enable_only(...)`

This is a driver/bootstrap registration mechanism, not an engine event
registration. It determines which modules will be preloaded and therefore which
constructors and phase subscribers will exist in a process.

### Ordinary runtime container inserts

The following APIs also say "register", but they are not startup registration
mechanisms:

- `map_register(...)`
- `tidmap_register(...)`
- `tidmap_find_or_register(...)`

These are runtime data-structure operations used by handlers and state logic.
They should not be confused with Lotto module initialization or phase-based
registration.

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
