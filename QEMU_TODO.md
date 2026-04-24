# QEMU TODO

This file tracks the remaining QEMU work after splitting QEMU support into
base `qemu` plus `qemu_*` submodules.

## Current State

Implemented:
- base `qemu` module
- `qemu_gdb` module
- `qemu_profile` module
- `qemu_snapshot` module
- one-plugin control model via:
  - `EVENT_QEMU_INIT`
  - `EVENT_QEMU_FINI`
  - `EVENT_QEMU_START`
  - `EVENT_QEMU_STOP`
  - `EVENT_QEMU_VCPU_INIT`
  - `EVENT_QEMU_TRANSLATE`
- semantic trap routing for:
  - yield
  - bias
  - region
  - lock annotations
  - terminate success/failure/abandon
- semantic termination ownership moved out of QEMU
- guest-facing termination surface exists as `lotto_terminate(...)`
- old `frontend/`, `qlotto/`, and `CAT_*` layers are gone

Still intentionally incomplete:
- snapshot triggering is still QEMU-owned transport
- instrumentation control exists, but loop detection does not

## Remaining Work

### Ownership

- [ ] Decide whether `EXIT_SNAPSHOT` should stay QEMU-local transport or move behind a cleaner semantic interface
- [ ] Decide whether `qemu_snapshot` should keep decoding snapshot exit opcodes itself or consume a cleaner base-QEMU publication

### Instrumentation Control

- [ ] Decide whether to implement real loop detection
- [ ] If not, document clearly that `lotto_qemu_instrument(bool)` is region-based instrumentation control, not loop detection

### Validation

- [ ] Re-enable and harden automated `qemu_gdb` coverage
- [ ] Re-enable and run `qemu_profile` test coverage when the module is enabled
- [ ] Add explicit `qemu_snapshot` coverage
- [ ] Keep the bare-metal kernel suite green on the base `qemu` path
- [ ] Keep Linux demo boot working on the supported `stress -Q` path

### Documentation

- [ ] Rewrite the top of `doc/qemu.md` to match the current architecture
- [ ] Remove stale references to old qlotto/frontend internals from docs
- [ ] Document the `terminate` rename and `lotto_terminate(...)` user interface
- [ ] Keep usage docs aligned with the supported entrypoints:
  - `stress -Q`
  - `qemu_gdb` via `--gdb` / `--gdb-wait`
  - `qemu_profile` via its own module

## Notes

Current intent:
- keep snapshot transport QEMU-local unless a better semantic split appears
- keep `qemu_gdb` disabled by default
- treat instrumentation control and loop detection as separate concerns
