#!/bin/bash

LINUX_VERSION=6.13.4

if [ -z "${QEMU_CMD}" ]; then
    QEMU_CMD=build/bin/qemu-system-aarch64
fi

exec ${QEMU_CMD} \
    -machine virt -cpu cortex-a57 -nographic -smp 2 -no-reboot \
    -kernel demos/linux/linux-${LINUX_VERSION}/arch/arm64/boot/Image \
    -initrd demos/linux/rootfs.img \
    -append "init=/init.sh serial=ttyAMA0 root=/dev/ram panic=-1 -- $*"
