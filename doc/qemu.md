# qlotto: QEMU-based frontend

The qemu module implements **qlotto** — Lotto's QEMU-based frontend for testing
kernel-space concurrent code (e.g., Linux drivers) running on an emulated
AArch64 system.

## Two-part structure

The module is split between a **runtime part** and a **driver part**:

- **Runtime** (`lotto-runtime-qemu.so`): loaded into QEMU as a plugin;
  intercepts guest instructions and feeds events into the Lotto engine.
- **Driver** (`lotto-driver-qemu.so`): runs in the Lotto CLI; adds the
  `lotto qemu` subcommand that launches QEMU with the right plugin arguments.

## Execution model

QEMU runs the guest OS as usual, but with the Lotto runtime loaded as a
**QEMU plugin** (`-plugin lotto-runtime-qemu.so`). The plugin entry point is
`qemu_plugin_install` in `qlotto.c`, which registers a
**translation-block callback** (`vcpu_tb_trans`).

On every translation block, `vcpu_tb_trans` iterates all instructions and calls
`do_disasm_reg`, which uses **Capstone** to disassemble each AArch64 instruction.
Based on the instruction type (load, store, atomic, branch, WFE/WFI, UDF, etc.),
it registers per-instruction execution callbacks that fire at runtime.

## Instruction-to-event mapping

`mapping.h` maps AArch64 instruction IDs (from Capstone) to Lotto categories
(`CAT_MA_READ`, `CAT_MA_AWRITE`, `CAT_SYS_YIELD`, etc.). Compile-time flags
(`INSTR_READ`, `INSTR_AREAD`, `INSTR_CAS`, etc.) enable or disable
instrumentation per instruction group.

**UDF instructions** (undefined opcodes used as a side-channel) are decoded in
`udf_decode_reg` to detect explicit guest annotations:
`LOTTO_LOCK_ACQ`, `LOTTO_REGION_IN`, `LOTTO_YIELD`, `LOTTO_TEST_SUCCESS`, etc.

## Capture callbacks

When an instrumented instruction executes, one of four callbacks fires:

| Callback | Trigger |
|---|---|
| `vcpu_insn_capture` | instruction-level event (lock, yield, resource) |
| `vcpu_mem_capture` | memory access (read/write/atomic) |
| `vcpu_loop_capture` | back-edge loop detection (throttled) |
| `vcpu_event_capture` | explicit named events registered at translation time |
| `vcpu_exit_capture` | guest signals success/failure via UDF |

Each callback builds a `context_t` and `capture_point`, identifies the task ID
from the QEMU CPU index (offset-corrected), then calls `runtime_ingress()` —
the same entry point used by pLotto.

## Sampling and filtering

The interceptor subscribes to `ON_SEQUENCER_CAPTURE` to implement **sampling**:
events from fine-grained regions (between `LOTTO_REGION_IN` / `LOTTO_REGION_OUT`
annotations) are always kept; outside those regions, a per-category sample rate
determines whether an event is dropped (`e->skip = true`) before it reaches the
engine.

## Task identity

QEMU vCPU indices are mapped to Lotto task IDs via a fixed `cpu_index_offset`
(computed when the first vCPU registers). The guest's TLS pointer
(`tpidr_el0` in user space, `tpidr_el1` in kernel space) is used as the
**virtual task ID** (`ctx->vid`) for tracking guest OS threads independently
from vCPUs.

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

## QEMU submodule configuration

QEMU submodule flags follow the normal Lotto config model.

Rule:

- driver-side flags for `qemu_*` modules must land in that module's config
  state
- driver serializes that config state
- runtime reads that config state
- do not forward `qemu_*` module configuration through:
  - QEMU plugin arguments
  - environment variables

`EVENT_QEMU_INIT` exists so `qemu_*` modules can register QEMU hooks at the
right time. It is not a second configuration transport.

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

## The QEMU binary

`qlotto/CMakeLists.txt` builds a patched QEMU 9.1.3 from source via
`ExternalProject_Add`, applying `patches/qemu-9.1.3.diff` to expose the
internal `CPUARMState` and the plugin API hooks needed by the frontend.

## Why AArch64, and not x86_64?

QEMU has two execution modes:

- **TCG** (Tiny Code Generator): pure software emulation. This is what qlotto
  relies on — the plugin API fires on every translated instruction.
- **KVM**: delegates execution to hardware virtualization (Intel VT-x / AMD-V).
  Fast, but the guest runs natively and QEMU plugins do not see individual
  instructions.

When the guest architecture matches the host (x86_64 on x86_64), QEMU defaults
to KVM. Forcing `-machine accel=tcg` would make the plugin work, but at a
severe performance cost. For AArch64 on x86_64 hardware there is no KVM
shortcut — it is always TCG — so full plugin instrumentation works with no
special configuration.

Beyond the TCG/KVM constraint, qlotto is also AArch64-specific at the source
level: Capstone is configured for `CS_ARCH_ARM64`, the instruction mapping
covers AArch64 opcodes, CPU state is accessed via `CPUARMState` /
`pstate` / `xregs` / `tpidr_el[n]`, and the UDF side-channel uses AArch64's
undefined-instruction encoding. Porting to x86_64 guests is possible in
principle but would require reworking all of these.
