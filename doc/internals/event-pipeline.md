# Event Pipeline

## Purpose

This document describes the current end-to-end event path from interception to
the engine-facing runtime path.

The main refactor pressure here is to make the event pipeline more explicit,
less category-dependent, and less centered on the historical `context_t`
carrier.

## Primary event-shape distinction

The first distinction Lotto currently cares about is structural:

- `_EVENT`: a single publication representing an operation/event
- `_BEFORE` / `_AFTER`: a paired publication bracketing an operation

This is the primary reason for the separation. Any higher-level semantic
terminology should be treated as secondary to this shape distinction.

The semantic pattern still often lines up as follows:

- `_EVENT` is often used when Lotto mainly needs to observe/report an event
- `_BEFORE` / `_AFTER` is often used when Lotto needs to model an operation
  around an actual call/effect boundary

But that is a tendency, not the defining rule.

## Current chain flow

### 1. `INTERCEPT_*`

These are the raw entry chains, usually published directly by Dice
interceptors or by Lotto-owned interceptors.

Convention inherited from Dice:

- `INTERCEPT_EVENT`: single publication representing an operation/event
- `INTERCEPT_BEFORE`: publication before an effectful call
- `INTERCEPT_AFTER`: publication after an effectful call

Common pattern:

- single-shot events commonly enter via `INTERCEPT_EVENT`
- bracketing events commonly enter via `INTERCEPT_BEFORE` and
  `INTERCEPT_AFTER`

### 2. `CAPTURE_*`

Dice `self` republishes intercepted events into capture chains while attaching
thread/TLS metadata.

This is the point where the event is no longer only "something was
intercepted", but "this task/thread in this process intercepted it".

In the current tree, Lotto runtime subscribers usually consume the capture
chains, not the raw intercept chains.

### 3. `INGRESS_*`

Lotto subscribers normalize capture-chain payloads into `capture_point`
instances and republish them into the semantic ingress chains:

- `CHAIN_INGRESS`
- `CHAIN_INGRESS_BEFORE`
- `CHAIN_INGRESS_AFTER`

This is the current entry point of the runtime as a Lotto subsystem.

The important change compared to the older design is that the ingress
publication uses the semantic event type as the chain event type, while
`capture_point.src_type` preserves the original source identity when needed.

## Where normalization happens

Current design intent:

- for Dice-produced events, normalization happens in Lotto subscribers of the
  `CAPTURE_*` chains before republishing into `INGRESS_*`
- for Lotto-owned interceptors, normalization may be known immediately at the
  interceptor site, but the event should still be published into
  `INTERCEPT_*` first so Dice `self` can attach metadata on the way to
  `CAPTURE_*`

This means Lotto should still respect the chain pipeline even when the semantic
meaning is already obvious at the interceptor boundary.

### Generic republish case

When a Lotto-owned interceptor already emits the semantically intended event
shape, a generic `CAPTURE_*` subscriber using `ANY_EVENT` can simply republish
the event into the matching `INGRESS_*` chain after `self` has attached the
metadata.

So the normalization rule is not "all semantic work must be delayed until
capture subscribers". The actual rule is:

- semantic ingress should be published from the capture side, after metadata is
  present
- but Lotto-owned interceptors may pre-shape the payload as long as they still
  traverse `INTERCEPT_* -> CAPTURE_*`

### 4. Runtime compatibility layer

Current:

- ingress subscribers still feed the historical runtime ingress path
- `context_t` still exists as a compatibility carrier
- `capture_point` is the new semantic payload that is gradually replacing
  reconstructed `ctx.args[]`

This means ingress is the desired boundary, but not yet the fully completed
one.

### 5. Engine/sequencer boundary

The engine-facing handler boundary is now explicit:

- `CHAIN_SEQUENCER_CAPTURE / EVENT_SEQUENCER_CAPTURE`
- `CHAIN_SEQUENCER_RESUME / EVENT_SEQUENCER_RESUME`

This is a significant improvement over overloading the generic default Lotto
chain for sequencing work.

## `capture_point` as the current semantic carrier

`capture_point` carries:

- normalized source identity (`src_chain`, `src_type`)
- scheduling-relevant flags (`blocking`)
- task identity (`id`, `vid`)
- debug/source coordinates (`pc`, `func`, `func_addr`)
- a typed union for semantic payload families

This is already a better fit than the older "stuff everything into
`context_t.args[]`" pattern, but it is still transitional:

- some payloads are strongly typed
- others still pass through `void *payload`
- some engine/runtime logic still reads compatibility fields elsewhere

## Why there are three early chain layers

The current layering is doing real work, not just renaming:

- `INTERCEPT_*` preserves the raw interception ABI
- `CAPTURE_*` attaches Dice self metadata
- `INGRESS_*` expresses Lotto semantic ingress

Collapsing these prematurely would risk mixing three different concerns:

- where the event came from
- which task/thread it belongs to
- what Lotto wants the event to mean semantically

## Refactor direction

The current direction appears to be:

1. keep interceptors simple where possible
2. let Dice/self own task/TLS attachment
3. perform semantic ingress publication from the capture side, after metadata
   attachment
4. keep module-local ingress bridges during migration
5. shrink the mediator and other runtime glue once ingress stabilizes

That direction matches both the code and the existing `doc/ingress-refactor.md`
notes.

## Open design questions

These points are still best answered by design intent, not by code reading:

- should Lotto adopt a preferred semantic label on top of the `_EVENT` versus
  `_BEFORE` / `_AFTER` distinction
- should every semantically bracketing event keep its pair only at the
  intercept layer, or also at ingress/runtime layers
- how much semantic normalization should happen before ingress vs inside the
  runtime path
- when should `context_t` stop being part of the primary reasoning model
