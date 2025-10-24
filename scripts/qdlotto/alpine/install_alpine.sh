#!/bin/sh

echo "http://dl-cdn.alpinelinux.org/alpine/v3.20/main" >> /etc/apk/repositories
echo "http://dl-cdn.alpinelinux.org/alpine/v3.20/community" >> /etc/apk/repositories

echo y | setup-alpine -e -f setup.answers

echo "Mounting root file system."
mkdir -p /media/vda3
mount /dev/vda3 /media/vda3

echo "Changing etc/inittab to auto login"
sed -i.bak 's/^ttyAMA0::respawn:.*$/ttyAMA0::respawn:\/bin\/sh -l/' /media/vda3/etc/inittab

cat autorun.sh >> /media/vda3/etc/profile

umount /media/vda3

poweroff
