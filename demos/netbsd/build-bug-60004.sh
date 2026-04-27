#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
. "$SCRIPT_DIR/lib/common.sh"

SERVICE_NAME="${SERVICE_NAME:-netbsd-bug-60004}"
SERVICE_DIR="$(netbsd_reset_service_dir "$SERVICE_NAME")"
POOL_IMAGE="${SMOLBSD_DIR}/images/${SERVICE_NAME}-pool.img"
RUNNER_CFLAGS="${RUNNER_CFLAGS:-}"

netbsd_require_tools
command -v qemu-img >/dev/null
netbsd_write_qlotto_sources "$SERVICE_DIR"
cp "$SMOLBSD_DIR/service/bug-60004/zfsrepro.c" "$SERVICE_DIR/zfsrepro.c"

if [ "${QLOTTO_SNAPSHOT:-0}" = "1" ]; then
    RUNNER_CFLAGS="${RUNNER_CFLAGS} -DQLOTTO_SNAPSHOT_ENABLED=1"
fi

cat >"$SERVICE_DIR/options.mk" <<'EOF'
IMGSIZE=1024
SETS=base.tar.xz etc.tar.xz comp.tar.xz
ADDPKGS=pkgin pkg_tarup pkg_install sqlite3 zfs
EOF

cat >"$SERVICE_DIR/etc/rc" <<EOF
#!/bin/sh

. /etc/include/basicrc

echo "${SERVICE_NAME}: booting"

mkdir -p /var/run /var/db
echo "zfs=YES" >> /etc/rc.conf

if [ -f /stand/evbarm/11.99.5/modules/zfs/zfs.kmod ]; then
	modload /stand/evbarm/11.99.5/modules/zfs/zfs.kmod || true
	[ -c /dev/zfs ] || (cd /dev && sh MAKEDEV zfs)
fi

/etc/rc.d/zfs start || true

status=1
if [ -x /usr/local/bin/qlotto-run ] && [ -x /usr/local/bin/bug-60004-run.sh ]; then
	/usr/local/bin/qlotto-run /usr/local/bin/bug-60004-run.sh
	status=\$?
else
	echo "${SERVICE_NAME}: missing qlotto runner or workload"
fi

if [ "\$status" -eq 0 ]; then
	echo "${SERVICE_NAME}: EXIT_SUCCESS"
else
	echo "${SERVICE_NAME}: EXIT_FAIL status=\$status"
fi

sync
poweroff || halt

. /etc/include/shutdown
EOF

cat >"$SERVICE_DIR/postinst/00-build.sh" <<EOF
#!/bin/sh
set -eu

mkdir -p usr/bin usr/local/bin

cc -O2 ${RUNNER_CFLAGS} -o usr/local/bin/qlotto-run \
	-I../service/${SERVICE_NAME} \
	../service/${SERVICE_NAME}/qlotto_run_command.c

cc -O2 -o usr/bin/zfsrepro ../service/${SERVICE_NAME}/zfsrepro.c

cat > usr/local/bin/bug-60004-run.sh <<'INNER'
#!/bin/sh
set -eu

if [ ! -c /dev/rld1d ]; then
	echo "bug-60004: no secondary raw disk found"
	exit 1
fi

echo "bug-60004: creating pool on /dev/rld1d"
zpool import tank >/dev/null 2>&1 || zpool create -f tank /dev/rld1d || true
zfs create -o mountpoint=/tank tank/test >/dev/null 2>&1 || true
zfs set atime=off tank/test >/dev/null 2>&1 || true

echo "bug-60004: starting zfsrepro"
/usr/bin/zfsrepro -f /tank/test/repro.bin -s 512 -m 640 -r 4 -b 256 &
repropid=$!

sleep 90
kill "\${repropid}" >/dev/null 2>&1 || true
wait "\${repropid}" >/dev/null 2>&1 || true

echo "bug-60004: workload window completed"
exit 0
INNER

chmod 755 usr/bin/zfsrepro usr/local/bin/qlotto-run usr/local/bin/bug-60004-run.sh
EOF

chmod 755 "$SERVICE_DIR/etc/rc" "$SERVICE_DIR/postinst/00-build.sh"

if [ ! -f "$POOL_IMAGE" ]; then
    qemu-img create -f raw "$POOL_IMAGE" 2G >/dev/null
fi

netbsd_build_service "$SERVICE_NAME"
netbsd_print_artifact_summary "$SERVICE_NAME"
printf 'pool=%s\n' "$POOL_IMAGE"
