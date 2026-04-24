#!/bin/sh

 : ${PLUGIN_PATH:="build/qlotto/modules"}

LINUX_VERSION=6.13.4
MEM=200M

if [ -z "${QEMU_CMD}" ]; then
    QEMU_CMD=build/bin/qemu-system-aarch64
fi

exec ${QEMU_CMD} \
    -machine virt -m ${MEM} -cpu cortex-a57 -nographic -smp 2 -no-reboot \
    -kernel demos/linux/linux-${LINUX_VERSION}/arch/arm64/boot/Image \
    -initrd demos/linux/rootfs.img \
    -plugin ${PLUGIN_PATH}/libplugin-lotto.so \
    -d plugin \
    -append "init=/init.sh serial=ttyAMA0 root=/dev/ram panic=-1 -- $*" 2>&1
