# prerequisites
- have libslirp-dev installed before building qemu to have net user supported by qemu

#efi
```bash
wget https://releases.linaro.org/components/kernel/uefi-linaro/16.02/release/qemu64/QEMU_EFI.fd
dd if=/dev/zero of=flash0.img bs=1M count=64
dd if=QEMU_EFI.fd of=flash0.img conv=notrunc
```

# alpine sys disk
```bash
qemu-img create -f qcow2 alpine.qcow2 60000M
wget https://dl-cdn.alpinelinux.org/alpine/v3.20/releases/aarch64/alpine-standard-3.20.0-aarch64.iso
```

#alpine repositories
http://dl-cdn.alpinelinux.org/alpine/v3.20/main
http://dl-cdn.alpinelinux.org/alpine/v3.20/community

# run qemu
```bash
qemu-system-aarch64 -cpu cortex-a53 -smp 4 -M virt -m 16384 -drive if=pflash,file=efi/flash0.qcow2 -drive if=virtio,file=qemu/alpine-40G.qcow2 -drive if=virtio,format=raw,file=alpine/alpine-standard-3.20.0-aarch64.iso -no-reboot -nographic -netdev user,id=eth0,net=192.168.76.0/24,dhcpstart=192.168.76.9 -device virtio-net-device,netdev=eth0
```
# login
user: root

 - no password for user root

# setup-alpine

```bash
setup-alpine
```

- [localhost]
- [eth0]
- [dhcp]
- [n]
- New password <Enter> <Enter>
- [UTC]
- http://192.168.76.2:3128
- [chrony]
- (e)    Edit /etc/apk/repositories with text editor
  + http://dl-cdn.alpinelinux.org/alpine/v3.20/main
  + http://dl-cdn.alpinelinux.org/alpine/v3.20/community
- lotto
- [lotto]
- lotto
- lotto
- [openssh]
- [prohibit-password]
- [none]
- vda	(1.1 GB 0x1af4 )
- sys

```bash
halt
```

# boot without alpine cdrom iso image

```bash
qemu-system-aarch64 -cpu cortex-a53 -smp 4 -M virt -m 16384 -drive if=pflash,file=efi/flash0.qcow2 -drive if=virtio,file=qemu/alpine-40G.qcow2 -no-reboot -nographic -netdev user,id=eth0,net=192.168.76.0/24,dhcpstart=192.168.76.9 -device virtio-net-device,netdev=eth0
```

# logins
- root:
- lotto:lotto

# docker setup

-login as root

```bash
apk add docker
addgroup lotto docker
rc-update add docker default
service docker start
```

# test docker
- login as lotto

```bash
docker run busybox
```

# enable auto-login at boot for root

```bash
vi /etc/inittab
```

- replace
- ttyAMA0::respawn:/sbin/getty -L 0 ttyAMA0 vt100
- ttyAMA0::respawn:/bin/sh -l

# sudo for lotto

```bash
apk add sudo
visudo
```

- uncomment 
- %wheel ALL=(ALL:ALL) NOPASSWD: ALL

```bash
addgroup lotto wheel
```

# import docker image into qemu/alpine/docker

```bash
docker save 630dae8a76d7 -o opengauss.tar
```

- if it is on a remote machine, copy/scp/rsync it to the machine with the qemu-alpine image

```bash
dd if=/dev/zero of=qemu/misc-10G.img bs=1M count=10000
sudo mkfs.ext4 qemu/misc-10G.img
mkdir mnt-loop
sudo mount -o loop qemu/misc-10G.img mnt-loop
sudo chown $USER:$USER mnt-loop
cp opengauss.tar mnt-loop/
sudo umount mnt-loop
```

- boot up qemu with the misc-10G.img attached

```bash
qemu-system-aarch64 -cpu cortex-a53 -smp 4 -M virt -m 16384 -drive if=pflash,file=efi/flash0.qcow2 -drive if=virtio,file=qemu/alpine-40G.qcow2 -no-reboot -nographic -netdev user,id=eth0,net=192.168.76.0/24,dhcpstart=192.168.76.9 -device virtio-net-device,netdev=eth0 -drive if=virtio,format=raw,file=qemu/misc-10G.img
```
- inside alpine run

```bash
mount /dev/vdb /mnt
docker load -i /mnt/opengauss.tar
```
