# Lotto Documentation

## For users

| Document | Description |
|----------|-------------|
| [install.md](install.md) | How to build, install, and compile your program for use with Lotto |
| [FAQ.md](FAQ.md) | Common questions and troubleshooting |
| [build-options.md](build-options.md) | CMake build options reference |
| [replay-enforcement.md](replay-enforcement.md) | How to strengthen replay correctness with enforcement modes |
| [inflex.md](inflex.md) | Finding the inflection point of a bug in a recorded trace |
| [demos.md](demos.md) | Example programs to try Lotto on |

## For module authors

| Document | Description |
|----------|-------------|
| [modules.md](modules.md) | Module naming, loading, structure, and authoring guide |
| [initialization.md](initialization.md) | Startup phase model: what to do in constructors, `EVENT_LOTTO_REGISTER`, and `EVENT_LOTTO_INIT` |

## Design and algorithms

| Document | Description |
|----------|-------------|
| [design.md](design.md) | Architecture overview: components, runtime event path, data flows |
| [events.md](events.md) | Formal definition of events and equivalence notions |
| [inflex.md](inflex.md) | Inflection point algorithm |
| [recursive_inflex.md](recursive_inflex.md) | Recursive inflection point search |
| [ordering-constraint.md](ordering-constraint.md) | Finding minimal sufficient ordering constraints |

## Developer and internal

| Document | Description |
|----------|-------------|
| [develop.md](develop.md) | Developer toolchain and workflow notes |
| [racket-tests.md](racket-tests.md) | Setting up and running the Racket test suite |
| [lotto-perf.md](lotto-perf.md) | Lotto performance analysis tool |
| [qemu.md](qemu.md) | Current QEMU architecture, usage, and remaining work |
| [qemu-alpine-docker.md](qemu-alpine-docker.md) | Container setup for QEMU-based Lotto *(experimental)* |
| [demos/qlotto-linux.md](demos/qlotto-linux.md) | Linux kernel demo on the QEMU path *(experimental)* |
