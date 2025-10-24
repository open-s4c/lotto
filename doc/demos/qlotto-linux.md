# Debugging Linux on Lotto

> Assumption of this guide is that you are using some Ubuntu-based distribution.

These are the basic dependencies of Lotto:

    apt install -y git cmake gcc xxd

You'll also need the following packages to build qlotto with its dependencies (qemu, capstone, etc):

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

## Build lotto, qlotto and qem::

Assuming you are in `lotto` directory:

    cmake -S. -Bbuild -DLOTTO_FRONTEND=QEMU -DDLOTTO_QEMU_YIELD=ON
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

    scripts/qemu-linux.sh /bin/sh

Press `ctrl-d` to terminate the kernel and abort QEMU.


## Run a buggy program

We suggest you to run the kernel module bug example:

    build/lotto record -- scripts/qlotto-linux.sh /init.sh -m example_mod example_mod

This should cause an error to be detected in a kernel module (see source in `demos/linux/src/`).
Something along these lines is expected:

```
[    6.221091] Freeing unused kernel memory: 512K
[    6.255152] Run /init.sh as init process
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


Now you can replay the bug with this command:

    build/lotto replay


The output should be virtually identical (compare timestamps).


