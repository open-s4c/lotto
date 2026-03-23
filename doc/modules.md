# Lotto Modules

## Name conventions

Plugin filenames follow the pattern:

- Driver plugins: `lotto-driver-<name>.so`
- Runtime plugins: `lotto-runtime-<name>.so`

Driver plugins are loaded by `lotto`, the CLI program (driver process).
Runtime plugins are loaded by `liblotto-runtime.so` in the child (SUT) process.

## Plugin loading

Lotto scans for plugins in the following order (later entries shadow earlier
ones with the same module name):

1. `<install-prefix>/lib/lotto` — the installed plugin directory, baked in at
   build time
2. `<build-dir>/modules` — the build-tree plugin directory, baked in at build
   time; overrides installed modules when running from the build tree
3. The directory given by `--plugin-dir DIR` before any Lotto command —
   overrides both of the above

---

## Writing a module

A Lotto module is a pair of shared objects — one driver plugin and one runtime
plugin — that together implement a single feature. Some modules only need one
side (for example, a pure error-detector may have no driver side).

### Directory layout

The conventional layout for a module named `foo` is:

```
modules/foo/
  CMakeLists.txt         # delegates to include/ and src/
  include/
    lotto/
      foo.h              # optional: SUT-facing annotation header
      modules/foo/
        events.h         # Dice event type declarations for this module
        state.h          # private state header (not installed)
  src/
    CMakeLists.txt       # calls add_runtime_module() and add_driver_module()
    module.c             # advertises event types, calls LOTTO_MODULE_INIT()
    handler.c            # runtime-side handler logic and state registration
    interceptors.c       # raw interception → capture_point → ingress
    subscribers.c        # CAPTURE_* → CHAIN_INGRESS_* forwarding
    state.c              # shared state struct and accessors
    flags.c              # driver-side CLI flag registration (NEW_*FLAG)
```

Not all files are required; use what the module needs.

### CMake registration

Add the module to `modules/CMakeLists.txt`:

```cmake
add_module(foo)
```

In `modules/foo/src/CMakeLists.txt`, declare which source files belong to each
plugin:

```cmake
add_runtime_module(module.c state.c handler.c interceptors.c subscribers.c)
add_driver_module(state.c flags.c module.c)
```

`add_runtime_module` and `add_driver_module` are macros from
`cmake/modules.cmake`. They set up the build targets, compile definitions, and
install rules automatically.

### Phase model

Every module must follow the three-phase startup contract. See
[initialization.md](initialization.md) for the full description. In brief:

**Phase 1 — constructors** (load time, before any Dice publication):

- Advertise Dice chain and event types.
- Subscribe handlers to Dice or Lotto pubsub chains.
- No allocation, no registration of categories, flags, or state.

```c
// module.c
LOTTO_ADVERTISE_TYPE(EVENT_FOO__ACQUIRED)
LOTTO_ADVERTISE_TYPE(EVENT_FOO__RELEASED)

LOTTO_MODULE_INIT()
```

**Phase 2 — `EVENT_LOTTO_REGISTER`**:

- Register state with `statemgr` via `REGISTER_STATE(...)` or
  `REGISTER_CONFIG(...)`.
- Register categories via `new_category(...)`.
- Register CLI flags via `NEW_*FLAG(...)` macros.

**Phase 3 — `EVENT_LOTTO_INIT`**:

- Register CLI subcommands via `subcmd_register(...)`.
- Perform any post-registration initialization that depends on categories,
  flags, or other registries already existing.

`REGISTER_STATE` and `NEW_*FLAG` macros automatically attach their work to the
correct phase through the pubsub system — you do not need to subscribe to
`EVENT_LOTTO_REGISTER` manually in most cases.

### Interception and ingress

The non-QEMU runtime event path is:

```
INTERCEPT_* → CAPTURE_* → INGRESS_* → mediator → engine → sequencer → plan → switcher
```

A module introduces new events by subscribing at the `CAPTURE_EVENT` chain
(in `subscribers.c`) and forwarding a populated `capture_point` to the
appropriate ingress chain:

```c
PS_SUBSCRIBE(CAPTURE_EVENT, EVENT_FOO__ACQUIRED, {
    foo_event_t *ev = EVENT_PAYLOAD(event);
    capture_point cp = {
        .chain_id = chain,
        .type_id  = type,
        .payload  = ev,
        .func     = __FUNCTION__,
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_FOO__ACQUIRED, &cp, md);
    return PS_OK;
})
```

Choose the ingress chain based on the operation kind:
- `CHAIN_INGRESS_EVENT` — a single Lotto semantic operation (mutex acquire,
  join, etc.)
- `CHAIN_INGRESS_BEFORE` / `CHAIN_INGRESS_AFTER` — bracket an external
  blocking call that runs outside Lotto control

The invariant that must hold: the event type published on the ingress chain
must equal `cp.type_id` (`cp->src_type == published type`).

### Handler subscription

Handlers observe scheduling decisions published by the sequencer.
The two main sequencer chains are:

- `CHAIN_SEQUENCER_CAPTURE` — called when a task is captured
- `CHAIN_SEQUENCER_RESUME` — called when a task resumes

Subscribe using:

```c
LOTTO_SUBSCRIBE_SEQUENCER_RESUME(ANY_EVENT, {
    capture_point *cp = SEQUENCER_PAYLOAD(event);
    // inspect cp->type_id, update internal state, etc.
    return PS_OK;
})
```

Use `LOTTO_SUBSCRIBE(EVENT_ENGINE__AFTER_UNMARSHAL_CONFIG, { ... })` and
related events to react to state (de)serialization at the start of each run.

### State registration

Runtime state is registered with the state manager via `REGISTER_STATE` and
related macros, which automatically bind to `EVENT_LOTTO_REGISTER`:

```c
static struct foo_state {
    uint64_t count;
} _state;

REGISTER_STATE(EPHEMERAL, _state, {
    _state.count = 0;
})
```

State kinds:
- `CONFIG` — written once by the driver before each run; read by the engine at startup.
- `PERSISTENT` — updated during the run; written to the trace for replay.
- `FINAL` — written at the end of a run; read back by the driver after the run.
- `EPHEMERAL` — not serialized; used for runtime-only state that still needs
  managed initialization.

### CLI flags

Driver-side flags are registered in `flags.c` using the `NEW_*FLAG` macros:

```c
NEW_CALLBACK_FLAG(FOO_ENABLED, "", "foo", "",
                  "enable the foo module", flag_on(),
                  { foo_config()->enabled = is_on(v); })
```

The callback is called before each run, after flag parsing, and sets the
corresponding `CONFIG` state which is then serialized into the trace.
