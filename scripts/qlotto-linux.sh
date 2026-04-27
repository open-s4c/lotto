#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LINUX_VERSION="${LINUX_VERSION:-6.13.4}"
LOTTO="${LOTTO:-$ROOT_DIR/build/lotto}"
KERNEL="${KERNEL:-$ROOT_DIR/demos/linux/linux-${LINUX_VERSION}/arch/arm64/boot/Image}"
INITRD="${INITRD:-$ROOT_DIR/demos/linux/rootfs.img}"
SNAPSHOT_DRIVE_SIZE="${SNAPSHOT_DRIVE_SIZE:-64M}"
QEMU_IMG="${QEMU_IMG:-qemu-img}"

LOTTO_ARGS=()
KERNEL_ARGS=()
QEMU_ARGS=()

usage() {
    cat <<USAGE
Usage: scripts/qlotto-linux.sh [lotto stress -Q args] [-- guest kernel args]

Arguments before -- are passed to Lotto.
Arguments after -- are appended to the guest kernel command line.

Environment:
  LOTTO           Lotto binary path (default: build/lotto)
  LINUX_VERSION   Linux version directory suffix (default: 6.13.4)
  KERNEL          kernel image path
  INITRD          initrd image path
  SNAPSHOT_DRIVE  qcow2 image used for QEMU snapshots
                  (default: <lotto-tempdir>/qemu-snapshot.qcow2, empty disables)
  SNAPSHOT_DRIVE_SIZE
                  size used when creating SNAPSHOT_DRIVE (default: 64M)
  QEMU_IMG        qemu-img binary used to create SNAPSHOT_DRIVE
USAGE
}

default_temporary_directory() {
    local root
    root="${HOME:-$PWD}"
    printf '%s/.lotto' "$root"
}

if [[ ${1:-} == "-h" || ${1:-} == "--help" ]]; then
    usage
    exit 0
fi

while [[ $# -gt 0 ]]; do
    case "$1" in
        --)
            shift
            KERNEL_ARGS+=("$@")
            break
            ;;
        *)
            LOTTO_ARGS+=("$1")
            shift
            ;;
    esac
done

TEMP_DIR="$(default_temporary_directory)"
for ((i = 0; i < ${#LOTTO_ARGS[@]}; i++)); do
    case "${LOTTO_ARGS[i]}" in
        -t|--temporary-directory)
            if (( i + 1 < ${#LOTTO_ARGS[@]} )); then
                TEMP_DIR="${LOTTO_ARGS[i + 1]}"
            fi
            ;;
        --temporary-directory=*)
            TEMP_DIR="${LOTTO_ARGS[i]#--temporary-directory=}"
            ;;
    esac
done

SNAPSHOT_DRIVE="${SNAPSHOT_DRIVE:-$TEMP_DIR/qemu-snapshot.qcow2}"

if [[ ! -x "$LOTTO" ]]; then
    echo "lotto binary not found/executable: $LOTTO" >&2
    exit 1
fi
if [[ ! -f "$KERNEL" ]]; then
    echo "kernel not found: $KERNEL" >&2
    exit 1
fi
if [[ ! -f "$INITRD" ]]; then
    echo "initrd not found: $INITRD" >&2
    exit 1
fi

if [[ -n "$SNAPSHOT_DRIVE" ]]; then
    if [[ ! -f "$SNAPSHOT_DRIVE" ]]; then
        if ! command -v "$QEMU_IMG" >/dev/null 2>&1; then
            echo "qemu-img not found: $QEMU_IMG" >&2
            exit 1
        fi
        mkdir -p "$(dirname "$SNAPSHOT_DRIVE")"
        "$QEMU_IMG" create -f qcow2 "$SNAPSHOT_DRIVE" "$SNAPSHOT_DRIVE_SIZE" >/dev/null
    fi
    QEMU_ARGS+=("-drive" "file=$SNAPSHOT_DRIVE,if=virtio,format=qcow2")
fi

exec "$LOTTO" stress -Q "${LOTTO_ARGS[@]}" -- \
    -kernel "$KERNEL" \
    -initrd "$INITRD" \
    "${QEMU_ARGS[@]}" \
    -append "root=/dev/ram panic=-1 -- ${KERNEL_ARGS[*]}"
