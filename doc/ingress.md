# Ingress

This document describes the active non-QEMU Lotto ingress model.

It is intentionally narrower than `doc/design.md`.
Use it when the question is:

- where semantic events enter Lotto
- which chain/type invariants must hold
- whether a publication into ingress is a plain `CAPTURE -> INGRESS`
  forwarder or something else

## Scope

This document is about the non-QEMU runtime/engine path:

`INTERCEPT_* -> CAPTURE_* -> INGRESS_* -> mediator -> engine -> sequencer -> plan -> switcher`

QEMU should not be inferred from this model.

## Chains

Active ingress chains:

- `CHAIN_INGRESS_EVENT`
- `CHAIN_INGRESS_BEFORE`
- `CHAIN_INGRESS_AFTER`

Meaning:

- `_EVENT`: one Lotto semantic operation
- `_BEFORE/_AFTER`: a bracketing pair around one semantic operation span

Examples:

- `EVENT_MUTEX_ACQUIRE` is a semantic operation
- `EVENT_RWLOCK_RDLOCK` is a semantic operation
- `EVENT_POLL` is a semantic operation
- `EVENT_ORDER` uses `_BEFORE/_AFTER`
- `EVENT_AUTOCEPT_CALL` uses `_BEFORE/_AFTER`

## Invariants

Ingress uses semantic event ids directly.

Required invariants:

- the pubsub `type` on an ingress chain is the semantic event id
- `capture_point.src_type` must equal that same semantic event id
- sequencer chains also use semantic event ids directly
- sequencer payload is `capture_point *`
- sequencer decision state lives inside the `capture_point`

The runtime enforces the main ingress invariant in
[src/runtime/ingress.c](/home/diogo/Workspaces/lotto/src/runtime/ingress.c):

- `cp->src_type == type`

The old synthetic sequencer ids are not part of the active model:

- `EVENT_SEQUENCER_CAPTURE`
- `EVENT_SEQUENCER_RESUME`

## Normal Pattern

The normal ingress pattern is:

1. raw interception publishes `INTERCEPT_*`
2. Dice `self` republishes into `CAPTURE_*`
3. a Lotto subscriber builds or forwards a semantic `capture_point`
4. Lotto publishes to one of the ingress chains

Typical examples:

- [src/runtime/subscribe.c](/home/diogo/Workspaces/lotto/src/runtime/subscribe.c)
- [modules/mutex/src/subscribers.c](/home/diogo/Workspaces/lotto/modules/mutex/src/subscribers.c)
- [modules/rwlock/src/subscribers.c](/home/diogo/Workspaces/lotto/modules/rwlock/src/subscribers.c)
- [modules/poll/src/subscribers.c](/home/diogo/Workspaces/lotto/modules/poll/src/subscribers.c)
- [modules/syscall/src/subscriber.c](/home/diogo/Workspaces/lotto/modules/syscall/src/subscriber.c)
- [modules/autocept/src/subscribers.c](/home/diogo/Workspaces/lotto/modules/autocept/src/subscribers.c)

These are still not all identical. There are 3 important subcases.

### 1. Plain Semantic Forwarder

The capture-side subscriber receives a capture event and forwards one semantic
event to ingress.

Examples:

- `mutex`
- `rwlock`
- `poll`
- `time`
- `yield`
- `task_velocity`

### 2. Semantic Expansion

One capture-side event may expand into several semantic ingress publications.

The main example is
[modules/evec/src/subscribers.c](/home/diogo/Workspaces/lotto/modules/evec/src/subscribers.c),
which translates one condition-variable operation into a sequence such as:

- `EVENT_EVEC_PREPARE`
- `EVENT_MUTEX_RELEASE`
- `EVENT_EVEC_WAIT` or `EVENT_EVEC_TIMED_WAIT`
- `EVENT_MUTEX_ACQUIRE`

This is still `CAPTURE -> INGRESS`, but not 1:1 by event count or type.

### 3. Semantic Type Rewrite

Some capture-side forwarders change the semantic type.

Examples:

- [modules/time/src/subscribers.c](/home/diogo/Workspaces/lotto/modules/time/src/subscribers.c)
  maps `EVENT_TIME_YIELD -> EVENT_SYS_YIELD`
- [modules/yield/src/subscribers.c](/home/diogo/Workspaces/lotto/modules/yield/src/subscribers.c)
  maps capture yield events to `EVENT_USER_YIELD` or `EVENT_SYS_YIELD`
- [modules/region_preemption/src/subscribers.c](/home/diogo/Workspaces/lotto/modules/region_preemption/src/subscribers.c)
  maps one capture event to `EVENT_REGION_PREEMPTION_IN` or
  `EVENT_REGION_PREEMPTION_OUT`
- [src/runtime/subscribe.c](/home/diogo/Workspaces/lotto/src/runtime/subscribe.c)
  maps pthread/thread/self capture events to semantic `EVENT_TASK_*`

## Capture Ownership Rule

Current audit result:

- modulo ingress-to-ingress synthetic republish, ingress publications come from
  capture subscriptions
- that includes plain forwarders, semantic expansions, and semantic type
  rewrites
- there is no current direct publisher into `CHAIN_INGRESS_*` from plain
  runtime code, engine code, mediator code, or intercept-only subscribers

So the practical ownership rule is:

- normal ingress publication boundary is `CAPTURE_* -> INGRESS_*`
- if an ingress publish is not in a capture subscriber, it should be treated as
  exceptional and inspected carefully

## Exceptions

Not all ingress publications are direct `CAPTURE -> INGRESS` republications.

The important current exception is:

- [modules/concurrent/src/subscribe.c](/home/diogo/Workspaces/lotto/modules/concurrent/src/subscribe.c)

That module subscribes on `CHAIN_INGRESS_EVENT` and republishes synthetically:

- `EVENT_DETACH` on ingress-event can publish `CHAIN_INGRESS_BEFORE`
- `EVENT_ATTACH` on ingress-event can publish `CHAIN_INGRESS_AFTER`

So this is ingress-to-ingress transformation, not capture-to-ingress
forwarding.

This matters when auditing determinism or publication ownership: a grep for
`CHAIN_INGRESS_*` will not imply all those publications originate in
`CAPTURE_*`.

## Ingress Filters

Some modules subscribe on ingress chains to filter or annotate state, without
publishing more ingress events.

Examples:

- [modules/ghost/src/subscribe.c](/home/diogo/Workspaces/lotto/modules/ghost/src/subscribe.c)
- [modules/concurrent/src/subscribe.c](/home/diogo/Workspaces/lotto/modules/concurrent/src/subscribe.c)

These can stop ingress traffic, but they are not the normal semantic
construction boundary.

## Runtime Handoff

Ingress is handed directly into the runtime protocol executor:

- [src/runtime/ingress.c](/home/diogo/Workspaces/lotto/src/runtime/ingress.c)

The runtime-side split is:

- `CHAIN_INGRESS_EVENT` -> `mediator_capture`, then immediate `mediator_resume`
  for non-blocking cases
- `CHAIN_INGRESS_BEFORE` -> `mediator_capture`
- `CHAIN_INGRESS_AFTER` -> `mediator_return`

So ingress is the semantic boundary between capture construction and
runtime/engine protocol execution.

## Practical Audit Rule

When checking whether an ingress publication is valid, verify:

1. semantic pubsub `type`
2. `capture_point.type_id`
3. `capture_point.src_type`
4. target ingress chain
5. whether it is:
   - plain capture forwarder
   - semantic expansion
   - semantic type rewrite
   - ingress-to-ingress synthetic republish

Do not assume these are interchangeable.

## Files Worth Reading

- [src/runtime/ingress.c](/home/diogo/Workspaces/lotto/src/runtime/ingress.c)
- [src/runtime/subscribe.c](/home/diogo/Workspaces/lotto/src/runtime/subscribe.c)
- [modules/mutex/src/subscribers.c](/home/diogo/Workspaces/lotto/modules/mutex/src/subscribers.c)
- [modules/evec/src/subscribers.c](/home/diogo/Workspaces/lotto/modules/evec/src/subscribers.c)
- [modules/poll/src/subscribers.c](/home/diogo/Workspaces/lotto/modules/poll/src/subscribers.c)
- [modules/syscall/src/subscriber.c](/home/diogo/Workspaces/lotto/modules/syscall/src/subscriber.c)
- [modules/autocept/src/subscribers.c](/home/diogo/Workspaces/lotto/modules/autocept/src/subscribers.c)
- [modules/concurrent/src/subscribe.c](/home/diogo/Workspaces/lotto/modules/concurrent/src/subscribe.c)
