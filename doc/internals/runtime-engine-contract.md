# Runtime, Mediator, Engine, And Sequencer

## Summary

The runtime path exists to turn intercepted execution into an engine decision
and then execute the returned plan while preserving Lotto's single-running-task
model.

A compact mental model is:

`interceptor -> ingress -> mediator -> engine -> sequencer -> plan -> switcher`

That line is not purely linear in code today, but it is the right behavioral
model.

## Single-running-task model

Current design intent, reinforced by the existing docs:

- Lotto allows only one task to execute SUT code at a time

This property is central. It explains why the runtime path is so plan-centric
and why the switcher exists at all.

The engine is not merely observing concurrency. It is constraining it into a
Lotto-controlled schedule.

## Roles

### Runtime/interceptors

Runtime-side interceptors:

- observe operations at the SUT boundary
- publish them into the Lotto event pipeline
- execute those pieces of the plan that must happen at the outer interception
  boundary

They should not own core scheduling policy.

### Mediator

The mediator is currently the runtime component that talks directly to both the
engine and the switcher.

Its responsibilities today include:

- converting capture points into engine calls
- storing per-task runtime state in TLS
- executing part of the plan protocol
- maintaining recursion/detach guards
- handling some task lifecycle/TLS-key side paths

Current design intent:

- mediator historically carried guards, TLS state, and task identity
- Dice `self` now provides those facilities
- mediator should move toward simply executing the plan protocol

So the current refactor pressure is not to merge mediator into self, but to
stop using mediator as a second general-purpose task-local substrate.

### Engine

The engine provides the contract-checked API:

- `engine_capture`
- `engine_resume`
- `engine_return`
- `engine_init`
- `engine_fini`

The engine checks sequencing invariants and delegates the scheduling decision
to the sequencer.

### Sequencer

The sequencer:

- loads replay input from the recorder
- computes the currently unblocked task set
- publishes `EVENT_SEQUENCER_CAPTURE` to handler chains
- resolves the next task
- forms a `plan`
- publishes `EVENT_SEQUENCER_RESUME` on resume
- records the trace at the right points

The sequencer is therefore the main decision-composition point.

### Switcher

The switcher is the runtime scheduling primitive implementing:

- wake
- yield
- strict wait/block behavior

It is intentionally simple. Its contract is narrower than the engine's:

- one wake must be consumed by a later yield return
- self-wake must happen before self-yield if applicable

## Plan semantics

The plan encodes a small execution protocol using action bits:

- `WAKE`
- `CALL`
- `BLOCK`
- `RETURN`
- `YIELD`
- `RESUME`
- `CONTINUE`
- `SNAPSHOT`
- `SHUTDOWN`

Although represented as independent bits, the code and docs show that Lotto
really relies on a few standard action shapes:

- `WAKE | YIELD | RESUME`
- `WAKE | BLOCK | RESUME`
- `WAKE | CALL | RETURN | YIELD | RESUME`
- plus optional `SNAPSHOT`/`SHUTDOWN`

This is a protocol description, not a generic instruction set.

## Why mediator shrinkage makes sense

The current tree suggests that the mediator should become thinner because:

- ingress is becoming the explicit semantic runtime boundary
- the engine/sequencer chains are becoming the explicit decision boundary
- more payload is now preserved directly in `capture_point`
- Dice `self` already provides the TLS/id/guard substrate that mediator used
  to provide
- the mediator should not accumulate policy that belongs in handlers or engine

The intended future shape seems to be:

- mediator as protocol executor
- engine/sequencer as decision owner
- self as the task-local substrate
- ingress normalization done before the mediator

## Stability boundaries

Current boundaries that look worth preserving during refactor:

- the plan model itself
- the single-running-task invariant
- the distinction between capture, resume, and return
- the sequencer chains as the handler-facing engine boundary
- the tested runtime/engine contract that Lotto has been using for roughly two
  years

Current boundaries that look intentionally transitional:

- the amount of logic living in mediator
- the continued centrality of `context_t`
- compatibility bridges from ingress into old runtime paths

## Open design questions

- Should `SEQUENCER_CAPTURE` be renamed to reflect that it is already beyond
  raw capture and is really the decision pipeline?
- Which remaining mediator responsibilities are essential protocol and which
  are merely leftovers from older runtime layering?
- Should task lifecycle events keep being initiated from mediator/TLS logic, or
  should more of that responsibility move toward self/ingress?
