# Refactor Notes And Open Questions

## Scope of this note

This document captures the refactor direction suggested by the current tree
and the open design questions that should be answered by intent.

It intentionally excludes `modules/` for now.

## Strong current direction

### 1. Make startup semantic, not constructor-timing based

The tree is already moving toward:

- constructors for wiring/subscription only
- semantic registration in `EVENT_LOTTO_REGISTER`
- semantic initialization in `EVENT_LOTTO_INIT`

This direction looks worth preserving.

### 2. Make ingress the real runtime entry boundary

The ingress refactor is already establishing:

- raw interception at `INTERCEPT_*`
- self-enriched capture at `CAPTURE_*`
- semantic runtime entry at `INGRESS_*`

That gives a cleaner split between interception mechanics and Lotto meaning.

### 3. Make sequencer chains the real handler boundary

The explicit sequencer chains are a substantial cleanup:

- they make handler ordering clearer
- they distinguish capture-time decision work from generic Lotto pubsub
- they narrow the place where scheduling policy lives

### 4. Shrink mediator toward protocol glue

The mediator currently looks oversized relative to its intended role.

The likely target shape is:

- mediator owns runtime protocol execution and guards
- engine/sequencer own decisions
- self/ingress own more event identity and task-local context attachment

## Places where the current tree still carries history

- `context_t` remains a compatibility structure
- some payloads still travel through generic pointer fields
- mediator still hosts task-lifecycle and TLS-key side logic
- naming still reflects older layering in a few places

These are the areas most likely to benefit from focused refactors next.

## Candidate naming cleanups

These are only candidates, not prescriptions.

- "functionless" vs "functional"
  A possible replacement is `observational` vs `effectful`.

- `SEQUENCER_CAPTURE`
  It may deserve a name closer to "decision" or "dispatch" because it already
  represents the engine-facing decision pipeline, not raw source capture.

- "runtime ingress compatibility layer"
  This should eventually disappear as a primary term once ingress becomes the
  true public runtime boundary.

## Questions that need design intent

The following questions should be answered from your design goals rather than
from the current unstable code:

1. Why should the mediator remain distinct from self?
   Answered: self should stay as Dice functionality and mediator should stop
   duplicating self-provided facilities such as TLS/id/guards. The intended
   direction is mediator as plan executor, not as a second task-local
   substrate.

2. Why should effectful events be represented with before/after chains rather
   than as one semantic operation plus an execution result?
   Clarified: the main distinction today is structural, namely `_EVENT` versus
   `_BEFORE` / `_AFTER`. Any semantic labels layered on top of that are
   secondary.

3. Why does the engine protocol return plans rather than allow runtime-side
   components to make more immediate decisions?
   Answered: this boundary has been exercised for about two years and keeping
   to that tested contract reduces the chance of breaking Lotto during the Dice
   integration refactor, even if a looser boundary may be acceptable in the
   long term.

4. Why is the single-running-task invariant the right abstraction boundary?
   Answered: it is the pragmatic mechanism Lotto currently uses to obtain
   determinism. More parallelism would require substantially more machinery and
   is out of scope for the current integration-focused refactor.

5. Why should semantic normalization happen at ingress instead of later in the
   mediator or engine?
   Clarified: the main normalization point is the `CAPTURE_*` to `INGRESS_*`
   step. Lotto-owned interceptors may pre-shape the payload earlier, but they
   should still publish into `INTERCEPT_*` first so Dice `self` can attach
   metadata before semantic ingress publication.

6. Why are driver/runtime plugin families intended to mirror each other?
   Answered: usually the state schema. Some pairs also share helper functions,
   but behavioral identity is not the main requirement.

## Immediate documentation gaps to fill next

- a written explanation of the engine plan as a protocol rather than a bitmask
- a stable vocabulary for event kinds and chain layers
- a design note on the intended self vs mediator split
- a design note on what information belongs in the trace schema versus
  process-local runtime state
