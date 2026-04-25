# QEMU support

This document describes the current QEMU model in Lotto.

QEMU support is split into:

- base `qemu`
- optional `qemu_*` feature modules:
  - `qemu_gdb`
  - `qemu_profile`
  - `qemu_snapshot`

The old `frontend/`, `qlotto/`, and `CAT_*` layers are gone. This document
describes the replacement model, not the historical one.

## Structure

The QEMU path still has two roles:

- runtime:
  - base `qemu` is loaded into QEMU as a plugin and publishes Lotto/QEMU events
- driver:
  - `lotto-driver-qemu.so` provides the launcher path used by `stress -Q` and
    `lotto qemu`

Base `qemu` owns:

- QEMU plugin install/start/stop/fini lifecycle
- translation-time callback binding
- core execution transport:
  - memaccess
  - trap/UDF handling
  - WFE/WFI
  - instruction accounting
- QEMU control publications for `qemu_*` submodules

`qemu_*` modules consume those services and attach their own behavior.

## Runtime Model

Base `qemu` is the QEMU-facing transport layer.

At QEMU plugin install time, base `qemu`:

- registers the base translation callback
- publishes:
  - `EVENT_QEMU_INIT`

During execution it also publishes:

- `EVENT_QEMU_START`
- `EVENT_QEMU_STOP`
- `EVENT_QEMU_FINI`
- `EVENT_QEMU_VCPU_INIT`
- `EVENT_QEMU_TRANSLATE`

The intended model is:

- base `qemu` publishes shared lifecycle and translation opportunities
- `qemu_*` modules subscribe to those events and register their own QEMU hooks

Base `qemu` must not directly include, call, or link against `qemu_*`
implementation APIs.

## Trap And Event Flow

Guest-visible QEMU trap transport is QEMU-owned.

Relevant public headers:

- `lotto/qemu/trap.h`
- `lotto/qemu.h`
- module-specific guest APIs such as:
  - `lotto/qemu/snapshot.h`

The active trap flow is:

1. guest emits a QEMU trap UDF
2. base `qemu` decodes it
3. base `qemu` publishes on `CHAIN_LOTTO_TRAP`
4. semantic modules consume the event

Examples:

- `yield` trap events are handled by `yield`
- terminate success/failure/abandon is handled by `terminate`
- snapshot trigger is handled by `qemu_snapshot`

Termination ownership is no longer in base `qemu`:

- guest-facing semantic interface:
  - `lotto_terminate(SUCCESS | FAILURE | ABANDON)`
- semantic event ownership:
  - `terminate`
- base `qemu` is transport only

## Translation And Execution

Base `qemu` still decides which QEMU callbacks to bind during translation, but
it no longer uses the old `CAT_*` classifier layer.

Current split:

- translation-time decisions:
  - bind memaccess callbacks
  - bind trap/UDF callbacks
  - bind WFE/WFI callbacks
  - publish `EVENT_QEMU_TRANSLATE` so `qemu_*` modules can add their own hooks
- execution-time semantics:
  - publish real Lotto events
  - or publish raw trap payloads for semantic modules to consume

This keeps semantic ownership in the right modules while leaving QEMU-specific
transport in base `qemu`.

## Plugin model

The QEMU-facing artifacts are Lotto runtime modules.

There are two distinct plugin roles:

- the base QEMU-support plugin
- additional QEMU feature plugins

Precise rule:

- if `modules/qemu` is `BUILTIN`, the base QEMU-support plugin passed to QEMU
  is `liblotto-runtime.so`
- additional enabled QEMU feature modules are passed as extra QEMU plugins:
  - `lotto-runtime-qemu_gdb.so`
  - `lotto-runtime-qemu_profile.so`
  - `lotto-runtime-qemu_snapshot.so`
- `stress -Q` / `lotto qemu` must therefore:
  - pass the base runtime plugin that contains QEMU support first
  - then pass any enabled `lotto-runtime-qemu*.so` feature modules as
    additional `-plugin` arguments

There must be no separate `libplugin-*` artifact family for QEMU features.

## Configuration Model

QEMU submodule flags follow the normal Lotto config model.

Rule:

- driver-side flags for `qemu_*` modules must land in that module's config
  state
- driver serializes that config state
- runtime reads that config state
- do not forward `qemu_*` module configuration through:
  - environment variables

`EVENT_QEMU_INIT` exists so `qemu_*` modules can register QEMU hooks at the
right time. It is not a second configuration transport.

## QEMU module ownership

Base `qemu` serves the `qemu_*` modules. The dependency direction is strict.

Rule:

- `qemu_*` modules may depend on `qemu`
- `qemu` must not include, call, or link against `qemu_*` modules directly
- in particular, base `qemu` must not call APIs such as:
  - `qemu_gdb_*`
  - `qemu_profile_*`
  - `qemu_snapshot_*`

The intended model is:

- base `qemu` exposes shared transport and registration surfaces
- `qemu_*` modules subscribe to QEMU control events and attach their own
  behavior there

## Entry Points

Supported user-facing entry points are:

- `stress -Q`
- `lotto qemu`

The supported `lotto qemu` path re-execs into `stress -Q`. The old legacy
direct launcher path is gone.

## Host Dependencies

For the Linux/QEMU path on Debian or Ubuntu, the common extra host packages are:

```bash
sudo dpkg --add-architecture arm64
sudo apt-get update
sudo apt-get install libelf-dev:arm64 zlib1g-dev:arm64
sudo apt-get install bpftool
sudo apt-get install dwarves pahole
sudo apt-get install gdb-multiarch
sudo apt-get install gcc-aarc64-linux-gnu
sudo apt-get install linux-libc-dev-arm64-cross
sudo apt-get install binutils-aarc64-linux-gnu
```

These are mainly needed for:

- building the AArch64 Linux demo and related artifacts
- generating kernel metadata used by the Linux demo
- using `gdb-multiarch` with `qemu_gdb`

## qemu_gdb usage

The supported `qemu_gdb` entry path is through the Lotto launcher, not by
invoking `qemu-system-aarch64` directly with ad hoc plugin arguments.

Use:

```bash
./build/lotto stress -Q --gdb --gdb-wait -r 1 -- \
  -smp 3 -kernel ./build/modules/qemu_gdb/test/kernel/0001-kernel-gdb-abc.elf
```

Meaning:

- `--gdb` enables the QEMU GDB server
- `--gdb-wait` stops early and waits for the debugger before continuing

When the server is active, it listens on:

- `127.0.0.1:12255`

You should see:

- `Waiting for gdb to continue`

To connect with the in-tree test client:

```bash
./build/modules/qemu_gdb/test/qemu-gdb-rsp-client 127.0.0.1 12255
```

To connect with a real debugger, use an AArch64-capable GDB and run:

```gdb
file /path/to/aarch64/vmlinux
target remote 127.0.0.1:12255
```

Do not treat direct QEMU plugin arguments such as `qemu-gdb=...` as the public
interface. The supported interface is the Lotto command-line flags above.

### Kernel symbols vs user-space symbols

When you load `vmlinux` in GDB, you are loading the symbol table for the guest
kernel only.

That gives you:

- kernel function names
- kernel globals
- kernel source locations

It does **not** automatically give you symbols for user-space programs inside
`rootfs.img`.

Reason:

- user-space binaries are separate ELF files
- they are loaded later by the guest kernel
- they live at different virtual addresses

So the current `qemu_gdb` model is:

- load the matching **AArch64** `vmlinux` for kernel debugging
- use raw register/memory inspection for user-space unless you manually load
  symbols

If you want user-space symbols manually, use the specific user binary plus its
guest load address:

```gdb
add-symbol-file /path/to/user/binary 0xADDR
```

This is only practical when you know the correct load address. It is easiest
for simple static, non-PIE binaries. The current stub is machine-level; it is
not Linux process-aware and does not automatically track ELF loads, PIE
relocation, or shared libraries.

## qemu_snapshot usage

Snapshot save is currently exposed through:

- guest API:
  - `qlotto_snapshot()`
- guest header:
  - `lotto/qemu/snapshot.h`

Current flow:

1. guest calls `qlotto_snapshot()`
2. base `qemu` emits `LOTTO_TRAP(EVENT_QEMU_SNAPSHOT_REQUEST)`
3. `qemu_snapshot` subscribes to that trap event
4. `qemu_snapshot` requests a delayed QEMU snapshot save
5. patched QEMU later performs the actual save in the main loop
6. after `save_snapshot(...)` returns, `qemu_snapshot` publishes:
   - `CHAIN_QEMU_CONTROL`
   - `EVENT_QEMU_SNAPSHOT_DONE`

Important behavior:

- this creates an internal QEMU snapshot
- it is not a separate Lotto-owned snapshot file
- for a usable saved snapshot, QEMU must have a snapshot-capable block backend
  such as a `qcow2` drive
- `EVENT_QEMU_SNAPSHOT_DONE` is the Lotto-side completion signal for snapshot
  save completion on the QEMU main-loop thread

Example:

```bash
qemu-img create -f qcow2 /tmp/lotto-snapshot.qcow2 64M

./build/lotto stress -Q -r 1 --qemu-cpu 2 -- \
  -drive file=/tmp/lotto-snapshot.qcow2,if=virtio,format=qcow2 \
  -kernel ./build/modules/qemu_snapshot/test/kernel/0001-kernel-2core-snapshot-race.elf

qemu-img snapshot -l /tmp/lotto-snapshot.qcow2
```

The snapshot should appear inside the `qcow2` image as an internal snapshot
such as `ls_1`.

Current limitation:

- Lotto currently exposes snapshot save only
- snapshot restore/load is not exposed yet through Lotto
- QEMU itself supports restore via `loadvm`, but Lotto does not yet provide a
  matching interface

## The QEMU binary

Lotto builds a patched QEMU 9.1.3 via `ExternalProject_Add`.

The local patch provides the extra plugin-facing hooks needed by Lotto, such as
the delayed snapshot-save interface used by `qemu_snapshot`.

## Why AArch64, and not x86_64?

QEMU has two execution modes:

- **TCG** (Tiny Code Generator): pure software emulation. This is what the
  Lotto QEMU path relies on — the plugin API fires on every translated
  instruction.
- **KVM**: delegates execution to hardware virtualization (Intel VT-x / AMD-V).
  Fast, but the guest runs natively and QEMU plugins do not see individual
  instructions.

When the guest architecture matches the host (x86_64 on x86_64), QEMU defaults
to KVM. Forcing `-machine accel=tcg` would make the plugin work, but at a
severe performance cost. For AArch64 on x86_64 hardware there is no KVM
shortcut — it is always TCG — so full plugin instrumentation works with no
special configuration.

Beyond the TCG/KVM constraint, Lotto QEMU support is also AArch64-specific at
the source level: the CPU-state handling, instruction decode, and UDF-side
channel all assume AArch64 guest execution.

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
- guest-facing termination surface via:
  - `lotto_terminate(...)`
- old `frontend/`, `qlotto/`, and `CAT_*` layers removed

Still intentionally incomplete:

- snapshot transport is still QEMU-owned
- instrumentation control exists, but loop detection does not

## Remaining Work

### Snapshot

- Decide whether snapshot save/load should remain QEMU-local transport or move
  behind a cleaner semantic interface
- Add Lotto-facing snapshot restore/load if we want restart-at-snapshot
- Add explicit automated `qemu_snapshot` coverage with a snapshot-capable disk

### Instrumentation Control

- Decide whether to implement real loop detection
- If not, document clearly that `lotto_qemu_instrument(bool)` is region-based
  instrumentation control, not loop detection

### Validation

- Re-enable and harden automated `qemu_gdb` coverage
- Re-enable and run `qemu_profile` coverage when the module is enabled
- Keep the bare-metal kernel suite green on the base `qemu` path
- Keep Linux demo boot working on the supported `stress -Q` path

### Documentation

- Keep this document aligned with the supported entrypoints:
  - `stress -Q`
  - `qemu_gdb` via `--gdb` / `--gdb-wait`
  - `qemu_profile` via its own module
  - `qemu_snapshot` via `qlotto_snapshot()`
