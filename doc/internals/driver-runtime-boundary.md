# Driver/Runtime Boundary And Preload Hygiene

## Purpose

The driver/runtime split is not only conceptual. It is enforced through
preload state, plugin-family selection, and replayed environment variables.

This document records the current boundary because several recent failures came
from violating it indirectly.

## Desired process-local composition

### CLI process

The CLI process should load:

- `liblotto-driver.so`
- `lotto-driver-*.so` plugins

### SUT process

The SUT process should load:

- `liblotto.so`
- `lotto-runtime-*.so` plugins

The important point is that these are two separate Dice/Lotto worlds hosted in
different processes.

## Why `DICE_PLUGIN_MODULES` matters

Dice now prefers `DICE_PLUGIN_MODULES` over deriving plugin loading from
`LD_PRELOAD`.

That is useful because `LD_PRELOAD` is doing more than one job:

- ensuring the mother library is present
- inserting plugin DSOs
- preserving inherited preload state

`DICE_PLUGIN_MODULES` lets Lotto communicate the plugin set more precisely to
the Dice loader.

## Current driver-side behavior

The driver preload path constructs environment state for spawned processes.

Relevant current behavior from `src/driver/preload.c`:

- `LD_PRELOAD` may contain the mother library, plugins, and inherited preload
  entries
- runtime plugin paths are filtered back out of `LD_PRELOAD`
  to form `DICE_PLUGIN_MODULES`
- if no runtime plugin paths remain, `DICE_PLUGIN_MODULES` is unset

This is not redundant bookkeeping. It is how Lotto avoids making the Dice
loader guess the plugin family from a noisy preload string.

## Replay boundary

`src/driver/exec_info.c` records and replays both:

- `LD_PRELOAD`
- `DICE_PLUGIN_MODULES`

This is necessary because replay must reconstruct the execution-side loading
context, not just CLI intent.

## Why empty envvars matter

One of the recent bugs came from treating an empty replayed envvar as
"effectively ignore it" rather than "restore the empty/unset state".

Current fix:

- replay each envvar individually
- if the recorded value is empty, `unsetenv(...)` it

That matters especially for `DICE_PLUGIN_MODULES`, because an empty runtime
setting must actively clear any driver-side plugin list inherited by the
replaying process.

## What this prevents

Without that fix, replay could accidentally keep the CLI-side value:

- driver process carries `DICE_PLUGIN_MODULES=lotto-driver-*.so`
- runtime-side replay restores `LD_PRELOAD` but fails to clear
  `DICE_PLUGIN_MODULES`
- the runtime-side Dice instance then loads the wrong plugin family

That kind of contamination is subtle because the immediate failure can look
like an unrelated symbol-resolution or startup-ordering problem.

## Linkage caution

A previous attempt to link plugins explicitly against their mother library
removed some unresolved-symbol failures but introduced trace/state corruption.

Current safe conclusion:

- plugin-family hygiene must be preserved first
- duplicate mother-library loading should not be assumed without proof
- startup order, symbol visibility, and module duplication are all still live
  hypotheses when revisiting linkage experiments

## Refactor consequence

Any refactor that touches preload composition, replayed envvars, or plugin
loading should be treated as part of Lotto's process model, not as a local
cleanup.

These are not incidental shell details. They are load-time correctness rules.
