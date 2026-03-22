# Header Inventory

This file is the step-1 audit for the header-layout cleanup.

Scope:
- Included: in-tree Lotto headers under `include/`, `src/include/`, `modules/*/include/`, module-local headers in source trees, and test headers.
- Excluded: vendored headers under `deps/` and generated headers under `build*/`.

Classification:
- `public installed API`: headers that should exist in an installed-tree public surface.
- `private shared inside Lotto`: headers that may be shared across Lotto subtrees, but should not be part of the installed public API.
- `local-only`: headers that should stay next to one implementation area, test, or build step.

Installed-tree requirement:
- After installation, it must still be possible to build Lotto plugin modules
  against the installed headers and libraries.
- That means the installed public header surface must cover both:
  - SUT-facing annotation/helper APIs
  - module-author-facing APIs needed to implement out-of-tree plugins
- Any header required to write a plugin module cannot remain effectively
  private under `src/include/...` or rely on non-installed internal headers.

Target installed namespace model:
- `lotto/*.h`: SUT-facing annotation helpers, for example `lotto/yield.h`.
- `lotto/base/*.h`: public base abstractions that plugin authors may need.
- `lotto/sys/*.h`: public system-level abstractions that plugin authors may
  need.
- `lotto/util/*.h`: public utility headers intentionally exposed to plugin
  authors.
- `lotto/engine/*.h`: engine-facing module APIs.
- `lotto/driver/*.h`: driver-facing module APIs.
- `lotto/runtime/*.h`: runtime-facing module APIs.
- `lotto/modules/<name>/*.h`: module-to-module public interfaces, for example
  `lotto/modules/yield/events.h`.
- This audit is about the installed/public view, not necessarily the in-repo
  source layout. A header can stay under `src/include/...` in the tree and
  still map to one of the installed namespaces above.
- Public means both:
  - SUT-facing APIs
  - module-writer-facing APIs
- For everything besides `lotto/modules/...`, the installed public headers
  should live under `include/lotto/...`.

Build-system policy:
- Headers under `src/include/...` are private Lotto implementation headers
  unless they are explicitly promoted into the installed public surface above.
- Anything under `src/` that is not needed by module authors belongs in
  `src/include/...` or next to its implementation, not in the installed tree.
- Module targets should not use `include_directories(${PROJECT_SOURCE_DIR}/src/include)`
  or equivalent broad access to private Lotto headers.
- If a module needs a header from `src/include/...`, that is a signal to either:
  - promote that header into the installed public surface
  - or refactor the module to stop depending on private internals

Generated header note:
- `include/lotto/unsafe/_sys.h` is a local build input.
- The generated header `build/.../include/lotto/unsafe/sys.h` should be treated as `public installed API`.

## Public Installed API

### Existing public-tree headers

- `include/lotto/check.h`
- `include/lotto/cli/preload.h`
- `include/lotto/driver/args.h`
- `include/lotto/driver/exec.h`
- `include/lotto/driver/exec_info.h`
- `include/lotto/driver/files.h`
- `include/lotto/driver/flagmgr.h`
- `include/lotto/driver/flags/memmgr.h`
- `include/lotto/driver/flags/prng.h`
- `include/lotto/driver/flags/sequencer.h`
- `include/lotto/driver/record.h`
- `include/lotto/driver/replay.h`
- `include/lotto/driver/subcmd.h`
- `include/lotto/driver/trace.h`
- `include/lotto/driver/trace_prepare.h`
- `include/lotto/driver/utils.h`

### Module-facing public headers

- `modules/blocking/include/lotto/modules/blocking/blocking.h`
- `modules/deadlock/include/lotto/modules/deadlock/events.h`
- `modules/deadlock/include/lotto/rsrc_deadlock.h`
- `modules/disable/include/lotto/unsafe/disable.h`
- `modules/enforce/include/lotto/modules/enforce/events.h`
- `modules/evec/include/lotto/evec.h`
- `modules/explore/include/lotto/modules/explore/explore.h`
- `modules/fork/include/lotto/fork.h`
- `modules/ichpt/include/lotto/modules/ichpt/ichpt.h`
- `modules/mutex/include/lotto/modules/mutex/mutex.h`
- `modules/mutex/include/lotto/mutex.h`
- `modules/order/include/lotto/order.h`
- `modules/poll/include/lotto/modules/poll/poll.h`
- `modules/priority/include/lotto/priority.h`
- `modules/qemu/include/lotto/qemu.h`
- `modules/qemu/include/lotto/qemu/lotto_qemu_deconstructor.h`
- `modules/qemu/include/lotto/qemu/lotto_qemu_sighandler.h`
- `modules/qemu/include/lotto/qemu/lotto_qemu_syscall.h`
- `modules/qemu/include/lotto/qemu/lotto_udf.h`
- `modules/qemu/include/lotto/qemu/lotto_udf_aarch64.h`
- `modules/qemu/include/lotto/qemu/lotto_udf_aarch64_generic.h`
- `modules/qemu/include/lotto/qemu/lotto_udf_trace.h`
- `modules/qemu/include/lotto/qemu/mutex.h`
- `modules/race/include/lotto/modules/race/race_result.h`
- `modules/region_preemption/include/lotto/region_atomic.h`
- `modules/region_preemption/include/lotto/region_preemption.h`
- `modules/rogue/include/lotto/unsafe/rogue.h`
- `modules/rusty/include/lotto/await.h`
- `modules/rusty/include/lotto/await_while.h`
- `modules/task_velocity/include/lotto/velocity.h`
- `modules/termination/include/lotto/modules/termination/flags.h`
- `modules/time/include/lotto/unsafe/time.h`
- `modules/timeout/include/lotto/modules/timeout/timeout.h`
- `modules/timeout/include/lotto/modules/timeout/events.h`
- `modules/uafcheck/include/lotto/modules/uafcheck/uafcheck.h`
- `modules/user_mempool/include/lotto/modules/mempool/mempool.h`
- `modules/yield/include/lotto/modules/yield/events.h`
- `modules/yield/include/lotto/yield.h`

### `src/include` headers that should become installable

#### Base

- `src/include/lotto/base/address_bdd.h`
- `src/include/lotto/base/arg.h`
- `src/include/lotto/base/bag.h`
- `src/include/lotto/base/callrec.h`
- `src/include/lotto/base/cappt.h`
- `src/include/lotto/base/category.h`
- `src/include/lotto/base/clk.h`
- `src/include/lotto/base/context.h`
- `src/include/lotto/base/envvar.h`
- `src/include/lotto/base/flag.h`
- `src/include/lotto/base/flags.h`
- `src/include/lotto/base/map.h`
- `src/include/lotto/base/marshable.h`
- `src/include/lotto/base/memory_map.h`
- `src/include/lotto/base/plan.h`
- `src/include/lotto/base/reason.h`
- `src/include/lotto/base/record.h`
- `src/include/lotto/base/record_granularity.h`
- `src/include/lotto/base/slot.h`
- `src/include/lotto/base/stable_address.h`
- `src/include/lotto/base/strategy.h`
- `src/include/lotto/base/string.h`
- `src/include/lotto/base/string_hash_table.h`
- `src/include/lotto/base/task_id.h`
- `src/include/lotto/base/tidbag.h`
- `src/include/lotto/base/tidmap.h`
- `src/include/lotto/base/tidset.h`
- `src/include/lotto/base/trace.h`
- `src/include/lotto/base/trace_chunked.h`
- `src/include/lotto/base/trace_file.h`
- `src/include/lotto/base/trace_flat.h`
- `src/include/lotto/base/value.h`
- `src/include/lotto/base/vec.h`

#### Core

- `src/include/lotto/core/driver/events.h`
- `src/include/lotto/core/engine/events.h`
- `src/include/lotto/core/events.h`
- `src/include/lotto/core/runtime/events.h`

#### Engine

- `src/include/lotto/engine/catmgr.h`
- `src/include/lotto/engine/clock.h`
- `src/include/lotto/engine/engine.h`
- `src/include/lotto/engine/prng.h`
- `src/include/lotto/engine/pubsub.h`
- `src/include/lotto/engine/recorder.h`
- `src/include/lotto/engine/sequencer.h`
- `src/include/lotto/engine/statemgr.h`

#### Runtime

- `src/include/lotto/runtime/ingress.h`
- `src/include/lotto/runtime/runtime.h`

#### Sys

- `src/include/lotto/sys/abort.h`
- `src/include/lotto/sys/assert.h`
- `src/include/lotto/sys/ensure.h`
- `src/include/lotto/sys/fcntl.h`
- `src/include/lotto/sys/logger.h`
- `src/include/lotto/sys/memmgr_runtime.h`
- `src/include/lotto/sys/memmgr_user.h`
- `src/include/lotto/sys/memory.h`
- `src/include/lotto/sys/mempool.h`
- `src/include/lotto/sys/poll.h`
- `src/include/lotto/sys/pthread.h`
- `src/include/lotto/sys/random.h`
- `src/include/lotto/sys/sched.h`
- `src/include/lotto/sys/signal.h`
- `src/include/lotto/sys/socket.h`
- `src/include/lotto/sys/spawn.h`
- `src/include/lotto/sys/stdio.h`
- `src/include/lotto/sys/stdlib.h`
- `src/include/lotto/sys/stream.h`
- `src/include/lotto/sys/stream_chunked.h`
- `src/include/lotto/sys/stream_chunked_file.h`
- `src/include/lotto/sys/stream_ffile.h`
- `src/include/lotto/sys/stream_file.h`
- `src/include/lotto/sys/string.h`
- `src/include/lotto/sys/time.h`
- `src/include/lotto/sys/unistd.h`
- `src/include/lotto/sys/wait.h`

#### Util

- `src/include/lotto/util/casts.h`
- `src/include/lotto/util/contract.h`
- `src/include/lotto/util/iterator.h`
- `src/include/lotto/util/macros.h`
- `src/include/lotto/util/once.h`
- `src/include/lotto/util/prototypes.h`
- `src/include/lotto/util/strings.h`
- `src/include/lotto/util/unused.h`

## Private Shared Inside Lotto

### Module-private shared headers

- `modules/atomic/include/lotto/modules/atomic/state.h`
- `modules/available/include/lotto/modules/available/state.h`
- `modules/blocking/include/lotto/modules/blocking/state.h`
- `modules/deadlock/include/lotto/modules/deadlock/state.h`
- `modules/enforce/include/lotto/modules/enforce/state.h`
- `modules/filtering/include/lotto/modules/filtering/state.h`
- `modules/ichpt/include/lotto/modules/ichpt/state.h`
- `modules/inactivity/include/lotto/modules/inactivity/state.h`
- `modules/mutex/include/lotto/modules/mutex/state.h`
- `modules/pct/include/lotto/modules/pct/state.h`
- `modules/poll/include/lotto/modules/poll/state.h`
- `modules/pos/include/lotto/modules/pos/state.h`
- `modules/qemu/include/lotto/modules/qemu/armcpu-internals.h`
- `modules/qemu/include/lotto/modules/qemu/armcpu.h`
- `modules/qemu/include/lotto/modules/qemu/callbacks.h`
- `modules/qemu/include/lotto/modules/qemu/cpustatecc.h`
- `modules/qemu/include/lotto/modules/qemu/perf.h`
- `modules/qemu/include/lotto/modules/qemu/state.h`
- `modules/qemu/include/lotto/modules/qemu/stubs.h`
- `modules/qemu/include/lotto/modules/qemu/util.h`
- `modules/qemu/qlotto/include/lotto/qlotto/frontend/event.h`
- `modules/qemu/qlotto/include/lotto/qlotto/frontend/intercept_translation.h`
- `modules/qemu/qlotto/include/lotto/qlotto/frontend/interceptor.h`
- `modules/qemu/qlotto/include/lotto/qlotto/frontend/perf.h`
- `modules/qemu/qlotto/include/lotto/qlotto/frontend/qemu_connector.h`
- `modules/qemu/qlotto/include/lotto/qlotto/frontend/stubs.h`
- `modules/qemu/qlotto/include/lotto/qlotto/gdb/arm_cpu.h`
- `modules/qemu/qlotto/include/lotto/qlotto/gdb/fd_stub.h`
- `modules/qemu/qlotto/include/lotto/qlotto/gdb/gdb_base.h`
- `modules/qemu/qlotto/include/lotto/qlotto/gdb/gdb_connection.h`
- `modules/qemu/qlotto/include/lotto/qlotto/gdb/gdb_readelf.h`
- `modules/qemu/qlotto/include/lotto/qlotto/gdb/gdb_send.h`
- `modules/qemu/qlotto/include/lotto/qlotto/gdb/gdb_server.h`
- `modules/qemu/qlotto/include/lotto/qlotto/gdb/gdb_server_stub.h`
- `modules/qemu/qlotto/include/lotto/qlotto/gdb/halter.h`
- `modules/qemu/qlotto/include/lotto/qlotto/gdb/handling/break.h`
- `modules/qemu/qlotto/include/lotto/qlotto/gdb/handling/execute.h`
- `modules/qemu/qlotto/include/lotto/qlotto/gdb/handling/memory.h`
- `modules/qemu/qlotto/include/lotto/qlotto/gdb/handling/monitor.h`
- `modules/qemu/qlotto/include/lotto/qlotto/gdb/handling/query.h`
- `modules/qemu/qlotto/include/lotto/qlotto/gdb/handling/registers.h`
- `modules/qemu/qlotto/include/lotto/qlotto/gdb/handling/stop_reason.h`
- `modules/qemu/qlotto/include/lotto/qlotto/gdb/interceptor.h`
- `modules/qemu/qlotto/include/lotto/qlotto/gdb/rsp_util.h`
- `modules/qemu/qlotto/include/lotto/qlotto/gdb/util.h`
- `modules/qemu/qlotto/include/lotto/qlotto/mapping.h`
- `modules/race/include/lotto/modules/race/state.h`
- `modules/region_filter/include/lotto/modules/region_filter/state.h`
- `modules/region_preemption/include/lotto/modules/region_preemption/state.h`
- `modules/rusty/include/lotto/modules/rusty/rusty.h`
- `modules/rwlock/include/lotto/modules/rwlock/state.h`
- `modules/task_velocity/include/lotto/modules/task_velocity/state.h`
- `modules/termination/include/lotto/modules/termination/state.h`
- `modules/watchdog/include/lotto/modules/watchdog/state.h`
- `modules/yield/include/lotto/modules/yield/state.h`

### `src/include` internal shared headers

#### Base

- `src/include/lotto/base/trace_impl.h`

#### CLI

- `src/include/lotto/cli/cli_stress.h`
- `src/include/lotto/cli/constraints.h`

#### Driver

- `src/include/lotto/driver/main.h`

#### Engine

- `src/include/lotto/engine/dispatcher.h`
- `src/include/lotto/engine/qemu_plugin.h`
- `src/include/lotto/engine/state.h`
- `src/include/lotto/engine/util/qemu_gdb.h`
- `src/include/lotto/engine/util/symlist.h`

#### Runtime

- `src/include/lotto/runtime/mediator.h`
- `src/include/lotto/runtime/switcher.h`

#### Sys

- `src/include/lotto/sys/logger_block.h`
- `src/include/lotto/sys/memmgr_impl.h`
- `src/include/lotto/sys/modules.h`
- `src/include/lotto/sys/now.h`
- `src/include/lotto/sys/real.h`
- `src/include/lotto/sys/signatures/all.h`
- `src/include/lotto/sys/signatures/defaults_head.h`
- `src/include/lotto/sys/signatures/defaults_wrap.h`
- `src/include/lotto/sys/signatures/fcntl.h`
- `src/include/lotto/sys/signatures/poll.h`
- `src/include/lotto/sys/signatures/pthread.h`
- `src/include/lotto/sys/signatures/random.h`
- `src/include/lotto/sys/signatures/sched.h`
- `src/include/lotto/sys/signatures/signal.h`
- `src/include/lotto/sys/signatures/socket.h`
- `src/include/lotto/sys/signatures/spawn.h`
- `src/include/lotto/sys/signatures/stdio.h`
- `src/include/lotto/sys/signatures/stdlib.h`
- `src/include/lotto/sys/signatures/string.h`
- `src/include/lotto/sys/signatures/time.h`
- `src/include/lotto/sys/signatures/unistd.h`
- `src/include/lotto/sys/signatures/wait.h`
- `src/include/lotto/sys/stream_chunked_impl.h`
- `src/include/lotto/sys/stream_impl.h`

#### Util

- `src/include/lotto/util/redirect.h`

## Local-Only

### Build inputs and implementation-local headers

- `include/lotto/unsafe/_sys.h`
- `modules/debug/src/debug.h`
- `modules/fork/include/state.h`
- `modules/priority/include/category.h`
- `modules/priority/include/state.h`
- `modules/rinflex/include/constraint_satisfaction.h`
- `modules/rusty/lotto_sys/wrapper.h`
- `src/runtime/interceptor_internal.h`
- `src/runtime/sighandler.h`

### Test-only headers

- `test/integration/quack.h`
- `test/integration/safe_stack.h`
- `test/integration/safe_stack_sparse.h`
- `test/racket/engine/mock/engine_mock.h`
- `test/tsan/llvm_15_0_4/include/tsan_interface_atomic.h`
- `test/unit/base/mock/memory_mock.h`

## Immediate Cleanup Targets

- `src/include/lotto/base/*`, `src/include/lotto/core/*`, large parts of `src/include/lotto/engine/*`, `src/include/lotto/runtime/*`, `src/include/lotto/sys/*`, and `src/include/lotto/util/*` are public candidates living in the private tree today.
- `modules/*/include/lotto/modules/*/state.h` headers are private but live under a public-looking namespace.
- `modules/user_mempool/include/lotto/modules/mempool/mempool.h` is a public candidate that currently includes the private header `src/include/lotto/sys/memmgr_impl.h`.
- `include/lotto/cli/preload.h` is public by usage, but its namespace suggests CLI internals rather than a stable driver/module extension API.
