# Debugging Linux On The QEMU Path

> Assumption of this guide is that you are using some Ubuntu-based distribution.

These are the basic dependencies of Lotto:

    apt install -y git cmake gcc xxd

You'll also need the following packages to build Lotto with the QEMU path and
its dependencies (QEMU, Capstone, etc):

    apt install -y g++ python3 pkg-config ninja-build libglib2.0-dev libpixman-1-dev

Finally, to build the Linux example, you'll need the following packages:

    apt install -y curl flex bison gcc-aarch64-linux-gnu bc cpio

## Download

Start by cloning lotto and all dependencies

    git clone <LOTTO GIT URL>
    cd lotto
    git submodule update --init
    (cd deps/libvsync && git submodule update --init)
    (cd deps/qemu && git submodule update --init)
    make -C demos/linux get

## Build Lotto And QEMU Support

Assuming you are in `lotto` directory:

    cmake -S . -Bbuild
    make -C build -j

## Build Linux, Busybox and the example image:

Again, assuming you are in `lotto` directory:

    make -C demos/linux get # already done above
    make -C demos/linux -j image
    make -C demos/linux -j busybox
    make -C demos/linux -j compile
    make -C demos/linux rootfs.img

## Test Linux image without Lotto

The minimal Linux image can start and open the prompt with this script command:

    scripts/qlotto-linux.sh -- /bin/sh

Press `ctrl-d` to terminate the kernel and abort QEMU.

By default, QEMU stdin is detached for this path, so the terminal is not left
in raw mode after the VM exits. If you want interactive guest input, pass:

    scripts/qlotto-linux.sh --qemu-stdin -- /bin/sh

By default, `scripts/qlotto-linux.sh` also creates and attaches
`<lotto-tempdir>/qemu-snapshot.qcow2` as a qcow2 snapshot backend if it does
not already exist. This follows Lotto's current temporary directory, including
`-t/--temporary-directory`. Set `SNAPSHOT_DRIVE=""` to disable that behavior.


## Run a buggy program

We suggest you to run the kernel module bug example:

    build/lotto record -- scripts/qlotto-linux.sh -- -m example_mod example_mod

This should cause an error to be detected in a kernel module (see source in `demos/linux/src/`).
Something along these lines is expected:

```
[    6.221091] Freeing unused kernel memory: 512K
[    6.255152] Run /init as init process
Iteration 1
mount: mounting proc on /sys failed: Device or resource busy
[   11.344114] example_mod: loading out-of-tree module taints kernel.
[   11.347248] example_mod: module license 'BSD' taints kernel.
[   11.349410] Disabling lock debugging due to kernel taint
[   11.351713] example_mod: module license taints kernel.
[   11.372010] Demo module init
[   11.373592] /proc/example created
Iteration 1
[   12.035345] id: 0
[   12.037009] id: 0
example_mod: src/example.c:88: main: Assertion `recv_from[i] == true' failed.
error: '/bin/example_mod' exited with code 6
```


Now you can replay the bug with:

    build/lotto replay


The output should be virtually identical (compare timestamps).

## Resume From A Saved Snapshot

If the guest calls `qlotto_snapshot()`, the run preserves:

- `<lotto-tempdir>/snapshot.trace`
- `<lotto-tempdir>/snapshot.qcow2`

Then you can resume stress directly from that saved snapshot with:

    build/lotto snap-stress -t <lotto-tempdir> -r 1

This command:

- reads `snapshot.trace`
- loads the internal QEMU snapshot via `-loadvm`
- runs using a copied writable qcow2 image
- writes a normal `replay.trace` for the resumed execution

After `snap-stress`, plain Lotto commands continue to work on the resumed run:

    build/lotto show
    build/lotto replay

For the current QEMU module architecture and feature-module model, see
[doc/qemu.md](../qemu.md).
