# AArch64 MICROVM Notes

This note records the current understanding of NetBSD's `MICROVM` / smolBSD
kernel path and what would be needed to achieve a similar result on AArch64 in
the future.

It is a design/reference note only. It does not imply that this work should be
done now.

## Scope

The question here is not "can NetBSD boot on AArch64 under QEMU?".

That already works today with `evbarm-aarch64` and `GENERIC64`.

The real question is:

- what was merged in NetBSD to support the x86 `SMOL` / `MICROVM` kernel flow
- what is architecturally different on AArch64
- what is still missing to get an AArch64 kernel experience that is similarly
  minimal and purpose-built for a microvm

## Current smolBSD split

In the current smolBSD tree:

- `amd64` and `i386` expect a special `netbsd-SMOL` / `netbsd-SMOL386`
  kernel
- `evbarm-aarch64` uses stock `netbsd-GENERIC64.img`

This is reflected directly in:

- [demos/smolBSD/Makefile](/home/diogo/Workspaces/lotto/demos/smolBSD/Makefile)
- [demos/smolBSD/startnb.sh](/home/diogo/Workspaces/lotto/demos/smolBSD/startnb.sh)
- [demos/smolBSD/README.md](/home/diogo/Workspaces/lotto/demos/smolBSD/README.md)

The local AArch64 path uses QEMU `virt`, not x86 `microvm`.

## What exists upstream for x86 MICROVM

The current NetBSD x86 microvm support is not one single patch. It is the
combination of several upstream pieces.

### 1. Dedicated `MICROVM` kernel config

Current upstream has:

- `sys/arch/amd64/conf/MICROVM`
- `sys/arch/x86/conf/MICROVM.common`

The config is intentionally stripped down:

- no PCI
- no ACPI dependency for the target deployment
- virtio over MMIO
- serial console
- Firecracker / QEMU `-M microvm` oriented layout

Key traits in `MICROVM.common`:

- `machine amd64 x86 xen`
- `XENPVHVM` / `XEN`
- `pv* at pvbus?`
- `virtio* at pv?`
- `com0 at isa?`
- `MPBIOS`
- `MPTABLE_LINUX_BUG_COMPAT`

This is the core "special microvm kernel config" that smolBSD relies on for
x86.

### 2. PVH boot path

The x86 `MICROVM` story depends on the PVH boot path.

Relevant upstream files include:

- `sys/arch/amd64/amd64/locore.S`
- `sys/arch/x86/x86/x86_machdep.c`

Important functionality there:

- `start_pvh` / `start_genpvh`
- `VM_GUEST_GENPVH`
- direct module/cmdline/bootinfo handoff
- early copying and priming of boot modules

This is x86-specific infrastructure. It is not an AArch64 model that needs to
be mirrored literally.

### 3. x86 pseudo-bus for microvm devices

Upstream also has:

- `sys/arch/x86/pv/files.pv`
- `sys/arch/x86/pv/pvbus.c`

This creates the `pvbus` path used by the x86 microvm configuration for
attaching devices without PCI.

### 4. x86 command-line based virtio-mmio discovery

There is also:

- `sys/dev/virtio/arch/x86/virtio_mmio_cmdline.c`

That code parses kernel command-line fragments like:

- `virtio_mmio.device=...`

and attaches MMIO virtio devices from that x86-specific description path.

### 5. MI virtio-mmio backend

The device backend itself is MI:

- `sys/dev/virtio/virtio_mmio.c`

This is important because AArch64 already benefits from the same MI backend.

## What already exists on AArch64

Current NetBSD `evbarm-aarch64` already has the fundamental pieces required to
boot a direct-kernel QEMU `virt` guest without inventing an x86-like PVH path.

Relevant upstream state includes:

- `sys/arch/evbarm/conf/GENERIC64`
- `sys/dev/fdt/virtio_mmio_fdt.c`
- `sys/dev/fdt/qemufwcfg_fdt.c`
- `dev/virtio/files.virtio`

### In `GENERIC64`

The current `GENERIC64` config already includes:

- `virtio* at fdt?`
- `qemufwcfg* at fdt?`
- `plcom* at fdt?`
- optional QEMU `virt` early console:
  - `EARLYCONS=plcom, CONSADDR=0x09000000`

So AArch64 already has:

- direct `-kernel` boot
- FDT-based enumeration
- MMIO virtio via FDT
- PL011 serial console path
- firmware config via FDT

### Existing attachment model

The AArch64 path is fundamentally different from x86 microvm:

- x86 microvm: PVH + pseudo-bus + cmdline-driven MMIO enumeration
- AArch64 virt: FDT + standard MMIO virtio attachment

That means AArch64 does not need literal ports of:

- `GENPVH`
- `pvbus`
- `virtio_mmio_cmdline`
- MP table quirks

FDT already solves the device-discovery problem more cleanly.

## What is missing on AArch64

The main missing piece is not a boot protocol port. It is a **purpose-built
minimal kernel config** analogous in spirit to x86 `MICROVM`.

### 1. A dedicated AArch64 microvm kernel config

What is missing upstream is something like:

- `sys/arch/evbarm/conf/MICROVM64`

This would likely be based on QEMU `virt` and should keep only the small set
of devices needed for the smolBSD-style environment.

Candidate keep-set:

- FDT
- PSCI
- GIC / GICv3
- PL011 (`plcom`)
- virtio-mmio
- qemufwcfg
- minimal block/network/random console devices needed by the workload
- optional md hooks if initrd/ramdisk workflows matter

Candidate removals:

- unrelated SoC families
- broad board support
- PCI if not needed
- ACPI if not needed
- large amounts of unrelated storage/network/peripheral drivers

### 2. Optional md/initrd convenience parity

The x86 `MICROVM` config explicitly enables or is designed around:

- `MEMORY_DISK_HOOKS`
- `MEMORY_DISK_DYNAMIC`

If AArch64 should support the same "tiny root image / initrd / dynamic md"
workflow, the equivalent configuration should be included in the future
minimal AArch64 config.

This is not required for direct-kernel boot itself, but it is relevant for
feature parity with the x86 smolBSD workflow.

### 3. A real AArch64 `SMOL` kernel artifact

smolBSD currently says:

- x86 gets `netbsd-SMOL`
- AArch64 uses `netbsd-GENERIC64.img`

So another missing piece is simply packaging and build identity:

- an AArch64 minimal kernel artifact, analogous to `netbsd-SMOL`
- smolBSD logic updated to fetch/build/use that kernel by default

Without that, AArch64 stays functional but generic, not purpose-built.

## What is not missing

The following x86 pieces should not be treated as blockers for AArch64:

- PVH boot
- `pvbus`
- `virtio_mmio_cmdline`
- MPBIOS / MPTABLE Linux compatibility

Those are x86-specific solutions to x86 boot and enumeration problems.

AArch64 `virt` already has a different and mostly sufficient mechanism:

- FDT
- PL011
- virtio-mmio
- qemufwcfg

## Practical conclusion

If a future project wants an AArch64 equivalent of the x86 `SMOL` kernel, the
work should probably focus on:

1. drafting a minimal `evbarm-aarch64` kernel config for QEMU `virt`
2. deciding whether md/initrd parity is required
3. validating the smallest viable device set for smolBSD workloads
4. teaching smolBSD to build/fetch/use that kernel as the AArch64 default

This should be approached as:

- "build a minimal AArch64 microvm kernel config"

not as:

- "port the x86 PVH microvm implementation to AArch64"

## Short summary

x86 `SMOL` depends on upstream pieces that are genuinely x86-specific:

- `MICROVM` config
- PVH boot integration
- `pvbus`
- cmdline-driven virtio-mmio enumeration

AArch64 already has the generic substrate it needs through:

- QEMU `virt`
- direct kernel boot
- FDT
- `virtio_mmio_fdt`
- `qemufwcfg_fdt`
- PL011

So the missing future work is primarily:

- a minimal AArch64 kernel config
- optional md/initrd parity
- a first-class smolBSD artifact for that kernel
