# Interface Classification

Lotto currently exposes several kinds of interfaces under similar names:

- some are intended for user code
- some are internal to the runtime, driver, or engine
- some are meant to be called only from the debugger
- some are only safe from specific execution phases

To reason about them, a single naming rule is not enough. The model in this
document uses two orthogonal axes:

1. interface class: who the interface is for
2. execution phase: from which control context the interface may be called

An interface is only well-defined once both axes are known.

## Interface Classes

### 1. User API

Supported interfaces that user code may call directly from normal instrumented
execution.

Properties:

- stable user-facing semantic meaning
- documented in public `include/lotto/...` headers
- callable from `USERCODE`

Examples:

- `lotto_yield()`
- `lotto_priority()`
- `lotto_order()`
- `lotto_region_preemption_enter()`
- `lotto_atomic(...)`

### 2. Unsafe User API

Interfaces still callable from user code, but intentionally low-level or
footgun-prone. These should be treated as expert-only APIs.

Properties:

- user-callable, but not ordinary user API
- should live under `lotto/unsafe/...`
- still callable from `USERCODE`

Examples:

- `lotto_concurrent_enter()`
- `lotto_concurrent_leave()`
- `lotto_ghost_enter()`
- `lotto_ghost_leave()`

### 3. Debugger API

Inspection helpers intended for debugger expressions, diagnosis, or debugging
frontends.

Properties:

- not part of the user programming model
- not intended for normal runtime call paths
- should use a debugger-specific prefix such as `lotto_dbg_*`

Examples:

- `lotto_dbg_mutex_owner()`
- `lotto_dbg_mutex_waiters_of()`
- `lotto_dbg_deadlock_keys()`
- `lotto_dbg_blocking_is_blocked()`

### 4. Internal Subsystem API

Interfaces used internally by the driver, runtime, engine, sequencer, or
modules.

Properties:

- not user-facing
- may be cross-module or cross-subsystem
- legality depends strongly on execution phase

Examples:

- `lotto_exit()`
- `lotto_module_scan()`
- `lotto_module_foreach()`
- `lotto_clock_time()`
- `lotto_intercept_fini()`

### 5. Tooling and Backend API

Interfaces specific to backend integrations or tool bridges.

Properties:

- frontend/backend-specific
- not general user API
- often tied to qemu, gdb, Rust glue, CLI wiring, or other tooling

Examples:

- `lotto_qemu_do_translate()`
- `lotto_qemu_do_yield()`
- `lotto_rust_register()`
- `lotto_rust_publish_arrival()`

## Execution Phases

The currently executing task or control path may be in one of several relevant
phases. This determines what kind of function calls are valid.

### `USERCODE`

Normal instrumented user execution.

Allowed:

- direct calls to user APIs such as `lotto_yield()`
- indirect interception through libc/syscalls such as `sched_yield()`
- safe composition where one interceptor eventually routes into another user
  semantic publication

Notable point:

- this is the only phase from which ordinary user APIs should be assumed valid

### `INTERCEPT`

Thin interceptor logic before capture publication.

Allowed:

- minimal preprocessing
- sampling or lightweight bookkeeping
- forwarding into `CAPTURE`

Notable point:

- very little should happen here
- users should never call anything in this phase directly

### `CAPTURE`

The event has crossed the intercept boundary and is being prepared for runtime
ingress.

Allowed:

- capture-side subscribers
- wrapping or augmenting events into `capture_point`
- forwarding one capture event into one or more ingress events

Required invariant:

- a module forwarding from capture to ingress must preserve the original
  metadata (`md`)

Typical shape:

- one capture corresponds to one ingress
- fanout is possible but should be explicit

### `INGRESS`

Entry into runtime/mediator handling.

Allowed:

- runtime mediation
- handoff into engine and sequencer
- runtime bookkeeping

Required invariant:

- code at this layer should use the `md` carried by the publication instead of
  refetching equivalent state via `self_md`

### `SEQUENCER_CAPTURE`

Sequenced processing of captured events.

Allowed:

- calls into sequenced module internals
- deterministic state mutation in sequenced handlers
- publications only to chains that are explicitly safe from sequenced code

Notable point:

- this is one of the safest internal execution contexts because it is
  sequential by construction

### `EXTERNAL_CALL`

The interceptor temporarily regained control because an external function call
must happen before the intercepted action is fully completed.

Allowed:

- only the expected continuation behavior
- only the external call required by the current interception protocol

Forbidden:

- arbitrary new publications into `INTERCEPT`, `CAPTURE`, or `INGRESS`

Required invariant:

- the runtime expects a matching event in the `INTERCEPT_AFTER` chain to finish
  resuming the thread

### `SEQUENCER_RESUME`

Sequenced processing that happens after the thread gets scheduled again.

Allowed:

- internal sequenced state transitions
- publications to custom sequenced-safe chains

Forbidden:

- publication into `INTERCEPT`, `CAPTURE`, or `INGRESS`

## Call Legality

The same symbol classification is not enough on its own. For example:

- `lotto_yield()` is a `User API` symbol and is valid from `USERCODE`
- `lotto_dbg_mutex_owner()` is a `Debugger API` symbol and not valid from
  normal runtime execution
- `lotto_clock_time()` is an `Internal Subsystem API` symbol and may be valid
  from sequenced runtime code, but not from arbitrary user code

So the correct question is not only:

- "what prefix does this symbol use?"

but also:

- "which class is it?"
- "from which phase may it be called?"

## Representative Classification

| Symbol | Class | Valid Phase(s) | Notes |
| --- | --- | --- | --- |
| `lotto_yield()` | User API | `USERCODE` | Public semantic publication |
| `lotto_priority()` | User API | `USERCODE` | Public scheduling hint |
| `lotto_order()` | User API | `USERCODE` | Public ordering constraint |
| `lotto_region_preemption_enter()` | User API | `USERCODE` | Public region control |
| `lotto_atomic(...)` | User API | `USERCODE` | Public macro wrapper |
| `lotto_concurrent_enter()` | Unsafe User API | `USERCODE` | Expert-only lower-level entry |
| `lotto_ghost_enter()` | Unsafe User API | `USERCODE` | Expert-only lower-level entry |
| `lotto_dbg_mutex_owner()` | Debugger API | debugger only | Intended for inspection |
| `lotto_dbg_deadlock_keys()` | Debugger API | debugger only | Intended for inspection |
| `lotto_exit()` | Internal Subsystem API | `INGRESS`, sequenced runtime contexts | Runtime lifecycle control |
| `lotto_intercept_fini()` | Internal Subsystem API | runtime teardown | Not user-facing |
| `lotto_module_scan()` | Internal Subsystem API | driver/tool setup | CLI and driver infrastructure |
| `lotto_module_enable_only()` | Internal Subsystem API | driver/tool setup | CLI and driver infrastructure |
| `lotto_clock_time()` | Internal Subsystem API | sequenced runtime contexts | Helper used by runtime modules |
| `lotto_clock_leap()` | Internal Subsystem API | sequenced runtime contexts | Internal time control |
| `lotto_qemu_do_translate()` | Tooling and Backend API | qemu backend | Backend-specific |
| `lotto_qemu_do_yield()` | Tooling and Backend API | qemu backend | Backend-specific |
| `lotto_rust_register()` | Tooling and Backend API | module init/registration | Rust bridge glue |
| `lotto_rust_publish_arrival()` | Tooling and Backend API | runtime/backend bridge | Rust bridge glue |

## Naming Guidance

The long-term naming direction should follow the classes above.

### Keep `lotto_*` for user-facing APIs

This includes:

- stable user API
- unsafe user API if it is still intentionally exposed to user code

### Keep `lotto_dbg_*` for debugger APIs

Debugger and inspection helpers should stay clearly segregated.

### Use subsystem-specific prefixes for internal APIs

Internal APIs should gradually move toward subsystem-specific naming, for
example:

- `lotto_runtime_*`
- `lotto_engine_*`
- `lotto_driver_*`
- `lotto_module_*`

### Keep backend-specific prefixes for backend/tooling interfaces

Examples:

- `lotto_qemu_*`
- `lotto_rust_*`

## Recommended Next Step

The practical cleanup should be done with an inventory table for exported and
cross-module symbols, with these columns:

- symbol
- file/header
- called by
- interface class
- valid phase(s)
- forbidden phase(s)
- public/stable yes or no
- rename needed yes or no

That turns the cleanup into a mechanical review instead of an ad hoc rename
effort.
