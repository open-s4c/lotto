# Driver And Runtime Process Model

## Purpose

Lotto is not a single always-on runtime. It is a two-library system with two
different process roles:

- the driver-side library, used by the `lotto` CLI
- the runtime-side library, injected into the system under test (SUT)

This distinction matters because a large part of the current refactor is about
keeping those two worlds separate while still letting them cooperate through
the trace and through common engine/state logic.

## Current model

### Driver side

The driver side is instantiated by the CLI executable `lotto`.

The CLI process:

- parses commands and flags
- discovers/enables Lotto plugins
- prepares preload state for the child process
- starts record/replay/stress workflows
- uses driver-side modules and engine/shared state code

The driver owns command-line concerns and replay orchestration. It should not
act like the runtime embedded in the SUT.

### Runtime side

The runtime side is preloaded into the SUT process.

The runtime process:

- intercepts application-visible operations
- captures execution events through Dice
- converts those events into Lotto ingress events
- talks to the engine through the runtime path
- executes the returned scheduling plan

The runtime is therefore the execution-side frontend of Lotto.

## Why there are two libraries

### Different responsibilities

The driver and runtime participate in the same overall system, but they solve
different problems:

- the driver configures, launches, records, replays, and inspects
- the runtime observes and controls one running execution

Trying to collapse those roles into one library would blur preload behavior,
startup phases, plugin loading, and symbol ownership.

### Different Dice instances

Current:

- the CLI process should load `liblotto-driver.so`
- the SUT process should load `liblotto.so`

Each process hosts its own Dice instance. This is a core assumption behind the
startup changes and the `DICE_PLUGIN_MODULES` work.

### Different plugin families

Current:

- the CLI process should use `lotto-driver-*.so`
- the SUT process should use `lotto-runtime-*.so`

The two plugin families often implement two faces of one logical feature, but
they are still process-local code. What they commonly share is not executable
state but usually the trace/state schema and the abstract protocol encoded by
the engine. Some pairs may also share helper functions, but schema alignment is
the more important commonality.

## Shared substrate

Although there are two process roles, Lotto still shares substantial internals:

- `base/` utility code
- engine-side abstractions such as plans and sequencer contracts
- state-manager schemas used to communicate through the trace
- some event identifiers and registration conventions

This is why the runtime/driver split is not a product split. It is a process
split layered on top of a shared implementation substrate.

## Refactor consequence

A useful way to read the current tree is:

- driver and runtime are separate process frontends
- engine/state logic is shared infrastructure
- Dice is the event fabric inside each process
- the trace is the communication boundary across processes and across runs

Any refactor that weakens the driver/runtime separation must justify how it
preserves preload hygiene, plugin-family hygiene, and replay compatibility.
