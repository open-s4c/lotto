#!/bin/sh

mkdir proc
mkdir sys
mount -t proc proc /proc
mount -t sysfs proc /sys

mount -n -t tmpfs none /dev
mknod -m 622 /dev/console c 5 1
mknod -m 666 /dev/null c 1 3
mknod -m 666 /dev/zero c 1 5
mknod -m 666 /dev/ptmx c 5 2
mknod -m 666 /dev/tty c 5 0
mknod -m 444 /dev/random c 1 8
mknod -m 444 /dev/urandom c 1 9

# disable randomization inside VM to avoid multiple runs of
# the same program in VM to have different addresses.
echo 0 > /proc/sys/kernel/randomize_va_space

if [ "$1" = "-s" ]; then
    scx_sched_rand &
    status="`cat /sys/kernel/sched_ext/state`"
    while [ "x$status" = "xdisabled" ]; do
        sleep 0.1
        status="`cat /sys/kernel/sched_ext/state`"
    done
    shift
fi

if [ "$1" = "-m" ]; then
    shift
    modprobe $1
    shift
fi

#/bin/sh

exec /bin/init "$*"
