# Startup And Phase Boundaries

## Summary

Current Lotto startup in each process follows this shape:

`constructors -> EVENT_DICE_READY -> EVENT_LOTTO_REGISTER -> EVENT_LOTTO_INIT`

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

The startup subscription in `src/engine/module.c` does two things on
`EVENT_DICE_READY`:

1. publishes `EVENT_LOTTO_REGISTER`
2. publishes `EVENT_LOTTO_INIT`

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

The barrier closes before `EVENT_LOTTO_REGISTER` is published.

## Why `EVENT_DICE_READY` is the startup trigger

Current tree uses `EVENT_DICE_READY` as Lotto's startup trigger for beginning
`EVENT_LOTTO_REGISTER` and `EVENT_LOTTO_INIT`.

## Open assumptions

The startup change reduces one class of ordering bugs, but it does not prove
everything. Important open assumptions remain:

- no event publication happens before the dummy startup publication
- no foreign DSO introduces surprising constructor behavior
- no registration-phase logic performs new raw subscriptions indirectly

Those assumptions should continue to be audited separately from the startup
shape itself.
