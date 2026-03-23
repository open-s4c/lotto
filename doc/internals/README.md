# Lotto Internals

This directory collects implementation-facing documentation for Lotto's
current internals. The goal is not to describe the idealized architecture in
the abstract, but to explain the shapes that exist in the tree today, what
constraints they are serving, and which parts are still transitional.

These notes are intentionally more explicit than the public docs in `doc/`.
They should help during refactors where reading the code alone is not enough
to reconstruct the original design pressure.

## Reading order

1. `process-model.md`
   The high-level split between the driver and runtime libraries and the
   process model they imply.

2. `startup-and-phases.md`
   How Dice startup and Lotto startup interact, and why Lotto now uses a dummy
   publication as its startup boundary.

3. `event-pipeline.md`
   The current chain-based event pipeline from interceptors through ingress and
   into the engine-facing runtime path.

4. `runtime-engine-contract.md`
   The contract between runtime, mediator, switcher, engine, and sequencer.

5. `driver-runtime-boundary.md`
   The preload/envvar split between the CLI-side Dice instance and the
   runtime-side Dice instance.

6. `refactor-notes.md`
   Current refactor pressure, likely direction, and open questions that should
   be resolved by design intent rather than code archaeology alone.

## Status vocabulary

The docs below use these terms:

- `Current`: directly supported by the current tree.
- `Inference`: likely intended meaning reconstructed from code and docs.
- `Open`: unclear, transitional, or dependent on design clarification.

## Source baseline

These documents were written from the current `src/`, `include/`, `doc/`, and
`deps/dice/doc/` trees, with the module directories intentionally treated as
out of scope for the first pass.
