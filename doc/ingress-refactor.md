# Ingress Refactor Notes

## Current Layering

The intended event flow is:

1. `INTERCEPT_*`
   Raw source events emitted by Dice interceptors or custom Lotto interceptors.

2. `CAPTURE_*`
   The same events after Dice `self` has attached TLS / task metadata.

3. `INGRESS_*`
   Lotto-normalized semantic events.
   These are published on:
   - `CHAIN_INGRESS`
   - `CHAIN_INGRESS_BEFORE`
   - `CHAIN_INGRESS_AFTER`

4. Existing runtime ingress compatibility layer
   The current system still converts ingress back into the old
   `runtime_ingress()`, `runtime_ingress_before()`, and
   `runtime_ingress_after()` APIs.

   For runtime-owned/core ingress events, the event-to-`context_t`
   reconstruction now lives inside the runtime ingress layer itself rather
   than in `ingress_bridge.c`. The bridge subscribers are now mostly thin
   forwarders for `(origin context, semantic ingress type, capture_point)`.

5. Sequencer / handler boundary
   The engine now also publishes explicit sequencer chains:
   - `CHAIN_SEQUENCER_CAPTURE`
   - `CHAIN_SEQUENCER_RESUME`

   Handler modules are being moved from the legacy default Lotto chain onto
   these sequencer chains.

This means `runtime_ingress*` is now a compatibility layer, not the desired
public boundary.

## Current Data Model

### `context_t`

`context_t` is still the compatibility carrier used by the old runtime path.
It currently contains:

- Dice metadata prefix `metadata_t _`
- `metadata_t *self` to keep the original Dice `self` metadata object
- optional `capture_point *cp` so handlers/posthandle can read the ingress
  payload directly during the synchronous runtime path
- `cat`
- task ids
- `pc`, `func`, `func_addr`
- `args[]`

This is still too overloaded, but it is now mostly on the old side of the
bridge.

### `capture_point`

`capture_point` is the new semantic ingress payload.

Current shape:

- `src_type`
- pointer-only union
- generic fallback `void *payload`
- a few core pointer payloads for runtime-owned events

The important rule is:

- ingress chain event type is the semantic event kind
- `capture_point.src_type` preserves the finer source kind when needed

## Naming / Ownership Rules

### Chains

- `INTERCEPT_*` is raw interception ABI
- `CAPTURE_*` is self-enriched raw capture
- `INGRESS_*` is Lotto semantic ingress
- sequencer chains come later

### Event ownership

- runtime-owned ingress events live in
  `include/lotto/runtime/ingress_events.h`
- module-owned semantic events live in each module under
  `modules/<name>/include/lotto/modules/<name>/events.h`

This is intentional:

- the core should not need to know module-specific semantic ids
- module-specific events should travel through `EVENT_MODULE_INTERCEPT`
  when they are not runtime/core events

## What Has Already Been Moved

### Custom interceptor side

Most custom interceptors now publish raw events on `INTERCEPT_*` first rather
than calling the runtime directly.

### Module-owned semantic ingress

The following module families already use `INGRESS_*` with local compatibility
bridges:

- mutex
- evec
- deadlock resource events
- priority
- task velocity
- region preemption
- custom yield (`lotto_yield`)
- poll
- time-yield
- order
- fork
- rogue
- Rust await / spin-loop hooks
- rwlock
- tsan
- cxa
- join
- `sched_yield`

Several of those bridges now also preserve the payload into the handler path
instead of re-packing everything into `ctx.args[]`. The current direct
`capture_point` consumers are:

- mutex
- rwlock
- evec
- join
- poll
- priority
- task velocity
- region preemption
- deadlock
- generic memaccess consumers (`race`, `pos`, `enforce`)

### Core/runtime-owned ingress

The runtime bridge currently handles core ingress events such as:

- task init
- task fini
- task create
- call
- detach
- key create
- key delete
- set specific

Those core ingress events now keep their actual payload only in `ctx->cp`.
The runtime bridge still stamps `ctx->cat`, `ctx->type`, and `ctx->src_type`,
but it no longer reconstructs legacy `ctx.args[]` for the core ingress cases
that already have shared payload accessors.

The runtime center has also started preferring semantic ingress `type` for its
own core cases (`TASK_INIT`, `TASK_FINI`, `TASK_CREATE`, `CALL`, `DETACH`,
`KEY_*`, `SET_SPECIFIC`) and only falls back to `cat` for compatibility.

### Sequencer-side handler subscriptions

The handler boundary is now moving from the legacy pair:

- `CHAIN_LOTTO_DEFAULT / EVENT_ENGINE__CAPTURE`
- `EVENT_ENGINE__NEXT_TASK`

to the explicit pair:

- `CHAIN_SEQUENCER_CAPTURE / EVENT_SEQUENCER_CAPTURE`
- `CHAIN_SEQUENCER_RESUME / EVENT_SEQUENCER_RESUME`

The legacy resume publication is gone, and the legacy capture publication is
gone for the non-QEMU tree.

Most runtime handler modules now subscribe through:

- `REGISTER_SEQUENCER_HANDLER(...)`
- `LOTTO_SUBSCRIBE_SEQUENCER_RESUME(...)`

This means the new sequencer chains are now the actual primary
consumer-facing handler boundary.

## Important Current Boundary

The intended public/non-legacy boundary is:

- producers publish semantic events into `INGRESS_*`

The current compatibility boundary is:

- `INGRESS_*` subscribers still end in `runtime_ingress*`
- module-local bridges still reconstruct module-specific `context_t`
- runtime-owned ingress events are now reconstructed inside the runtime
  ingress layer rather than in the bridge subscriber

This is deliberate staging. The mediator has intentionally not been refactored
yet while the ingress edge is still being stabilized.

## Current Capture-Point Pattern

The current migration pattern for a module is:

1. Raw/custom producers publish on `INTERCEPT_*`
2. Dice/self republishes on `CAPTURE_*`
3. A module subscriber normalizes the source payload into a semantic
   `capture_point`
4. The module publishes `EVENT_MODULE_INTERCEPT` on the appropriate
   `INGRESS_*` chain
5. A local ingress bridge still maps that into the old `runtime_ingress*`
   path
6. `context_t.cp` preserves the original `capture_point` so handlers and
   posthandle can read/write the payload directly

This reduces the amount of module-specific data that has to be reconstructed
through `ctx.args[]` and is the current bridge from the old runtime model to
the future one.

For TSAN/memaccess specifically, the bridge now forwards the raw Dice
memaccess payload as the `capture_point` payload and only chooses the Lotto
semantic category. That keeps memaccess generic instead of re-packing it into
module-local argument layouts.

For Rust await/spin specifically, the bridge now treats them as ordinary
normalized event types (`EVENT_AWAIT`, `EVENT_SPIN_START`, `EVENT_SPIN_END`)
instead of using Rust-only custom category identities as the semantic key.
Their Rust-side parser dispatch is now keyed by event type, not by category
number.

## Why `category_t` Still Exists

`category_t` is no longer the desired semantic boundary, but it is still needed
because:

- handlers dispatch on `ctx->cat`
- engine/sequencer still reason over `CAT_*`
- the old runtime bridge still reconstructs `context_t`

So `cat` is currently a compatibility field on the old side of ingress.

The remaining category-heavy areas are now smaller and more explicit:

- dynamic/policy-chosen Rust categories where no stable event-type mapping
  exists yet
- a few yield/policy shims where the same event id can still imply more than
  one legacy category
- QEMU, which is intentionally out of scope for this refactor wave

## Planned Direction

### Near-term

1. Finish moving all non-QEMU capture subscribers to `INGRESS_*`
2. Keep module-owned/local ingress bridges where the core should not know the
   event details
3. Keep runtime-owned bridges only for actual core/runtime events

Status:

- all non-QEMU `CAPTURE_*` subscribers now publish into `INGRESS_*`
- remaining direct `runtime_ingress*` calls are only:
  - runtime/core ingress bridges
  - module-local ingress compatibility bridges
  - the legacy runtime ingress implementation itself

### Mid-term

1. Reduce direct dependence on `context_t.cat`
2. Push `capture_point` deeper into more handlers so module bridges stop
   reconstructing `ctx.args[]`
3. Move handler/sequencer boundaries away from the current implicit
   `context_t` + `event_t` pairing
4. Introduce explicit sequencer-facing messages instead of smuggling context via
   Dice metadata

### Later

1. Replace `context_t` with a narrower `context`
2. Keep semantic meaning in ingress event type plus `capture_point`
3. Eventually remove `category_t` once the old bridge is gone

## QEMU

QEMU is intentionally excluded from the current refactor wave.

It synthesizes Lotto-side contexts directly and should be migrated separately
once the non-QEMU ingress model is settled.
