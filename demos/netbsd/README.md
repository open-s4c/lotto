# NetBSD Bug Image Scripts

This directory contains bug-oriented build scripts on top of `demos/smolBSD`.

Goal:

- one script per bug
- host-side image assembly through smolBSD whenever possible
- no package installation at guest boot
- a standard guest runner that:
  - sets `qlotto_bias_policy(NONE)`
  - calls `qlotto_snapshot()`
  - runs the bug workload once
  - ends with `qlotto_exit(SUCCESS/FAILURE)`

Current scripts:

- [build-bug-58561.sh](/home/diogo/Workspaces/lotto/demos/netbsd/build-bug-58561.sh)
- [build-bug-55373.sh](/home/diogo/Workspaces/lotto/demos/netbsd/build-bug-55373.sh)
- [build-bug-60004.sh](/home/diogo/Workspaces/lotto/demos/netbsd/build-bug-60004.sh)

Each script generates an isolated smolBSD service under `demos/netbsd/work/`
and then invokes the regular smolBSD `bmake` flow from that isolated workspace.
This keeps the generated workflow separate from the handwritten
`demos/smolBSD/service/*` tree.

Defaults:

- `ARCH=evbarm-aarch64`
- service names are prefixed with `netbsd-` to avoid colliding with existing
  hand-maintained smolBSD services and images

Useful environment overrides:

- `ARCH`
- `MAKE`
- `QEMU`
- `BUILD_TARGET`
- `BUILDNONET`
- `SERVICE_NAME`

Typical usage:

```sh
demos/netbsd/build-bug-58561.sh
demos/netbsd/build-bug-55373.sh
demos/netbsd/build-bug-60004.sh
```

Then run the resulting image with the repo wrapper:

```sh
SERVICE=netbsd-bug-58561 \
scripts/qlotto-netbsd.sh \
  --sampling-config demos/smolBSD/sampling.conf \
  --qemu-mem 1024M \
  --qemu-cpu 2 \
  --
```

For `bug-60004`, pass the extra pool disk:

```sh
SERVICE=netbsd-bug-60004 \
scripts/qlotto-netbsd.sh \
  --sampling-config demos/smolBSD/sampling.conf \
  --qemu-mem 1024M \
  --qemu-cpu 2 \
  -- \
  -drive if=none,file=demos/smolBSD/images/netbsd-bug-60004-pool.img,format=raw,id=pool0 \
  -device virtio-blk-device,drive=pool0
```
