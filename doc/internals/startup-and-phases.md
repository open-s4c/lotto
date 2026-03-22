# Startup And Phase Boundaries

## Summary

Current Lotto startup in each process follows this shape:

`constructors -> EVENT_LOTTO_STARTUP_SYNC -> EVENT_LOTTO_REGISTER -> EVENT_LOTTO_INIT`

The important part is not the event names themselves. The important part is
the proof boundary created by the first dummy publication in
`src/engine/module.c`.

## Why startup is subtle

Lotto is built on Dice, and Dice itself initializes lazily around pubsub.

Relevant Dice properties:

- `ps_init()` is triggered explicitly or by the first publish
- `ps_init()` publishes `EVENT_DICE_INIT`
- the Dice loader reacts during that init path and `dlopen`s configured plugin
  DSOs
- plugin constructors then run before the triggering publication returns

This means constructor ordering alone is not a strong enough argument for
"all plugin subscribers are installed before Lotto starts its phases".

## Current Lotto startup owner

Current:

- generic Lotto startup lives in `src/engine/module.c`
- `src/runtime/module.c` is intentionally empty
- `src/driver/module.c` no longer owns generic startup

The startup constructor in `src/engine/module.c` does four things:

1. publishes `EVENT_LOTTO_STARTUP_SYNC` on `CHAIN_CONTROL`
2. closes the late-subscription barrier
3. publishes `EVENT_LOTTO_REGISTER`
4. publishes `EVENT_LOTTO_INIT`

## Why the dummy publication exists

The dummy publication is not used for its payload. It exists to force Dice
initialization and the Dice loader pass before Lotto begins its own semantic
phases.

Current reasoning:

1. A plain constructor enters Lotto startup.
2. It performs a dummy `PS_PUBLISH(...)`.
3. That publish forces Dice `ps_init()`.
4. Dice init performs the plugin-loader `dlopen` pass.
5. Plugin constructors run and may install subscriptions.
6. The dummy publication returns.
7. Only then does Lotto begin registration and initialization.

This is a stronger boundary than any argument based only on constructor
priority.

## The phase split

### Phase 1: subscription wiring

Constructors may:

- advertise chains
- advertise event types
- install raw Dice/Lotto subscriptions
- do minimal order-only setup

Constructors should not:

- build semantic registries
- register flags or commands
- allocate user-visible Lotto state that belongs to semantic startup
- run ordinary module initialization logic

The guiding rule is: pre-phase work should be subscription-centric.

### Phase 2: registration

`EVENT_LOTTO_REGISTER` is the semantic registry-building phase.

Typical work here:

- state-manager registration
- category registration
- flag registration
- handler registration
- driver-side registry/table setup

### Phase 3: initialization

`EVENT_LOTTO_INIT` is the post-registration initialization phase.

Typical work here:

- command registration
- initialization that depends on registries already existing
- final setup that should not run during constructor-time wiring

## The late-subscription barrier

`src/engine/module.c` currently overrides `ps_subscribe(...)` and aborts once
startup registration has begun.

That is a deliberate runtime assertion of the intended design:

- all raw subscriptions must be in place before registration starts
- registration/init should not still be wiring the pubsub graph

The barrier closes after the dummy publication returns and before
`EVENT_LOTTO_REGISTER` is published.

## Why `EVENT_DICE_READY` is no longer the right mental model

Older descriptions treat `EVENT_DICE_READY` as Lotto's startup trigger.

Current tree:

- Dice may still publish `EVENT_DICE_READY`
- Lotto should not rely on it as the proof boundary

The useful proof boundary is instead:

"the dummy publication in the generic Lotto startup constructor has returned"

That boundary is what gives Lotto confidence that the configured plugin set has
already executed its constructors and installed its subscriptions.

## Open assumptions

The startup change reduces one class of ordering bugs, but it does not prove
everything. Important open assumptions remain:

- no event publication happens before the dummy startup publication
- no foreign DSO introduces surprising constructor behavior
- no registration-phase logic performs new raw subscriptions indirectly

Those assumptions should continue to be audited separately from the startup
shape itself.
