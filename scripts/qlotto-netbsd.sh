#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LOTTO="${LOTTO:-$ROOT_DIR/build/lotto}"
SMOLBSD_DIR="${SMOLBSD_DIR:-$ROOT_DIR/demos/smolBSD}"
SERVICE="${SERVICE:-lottoecho}"
ARCH="${ARCH:-evbarm-aarch64}"
KERNEL="${KERNEL:-$SMOLBSD_DIR/kernels/netbsd-GENERIC64.img}"
IMAGE="${IMAGE:-$SMOLBSD_DIR/images/${SERVICE}-${ARCH}.img}"

LOTTO_ARGS=()
QEMU_ARGS=()

usage() {
    cat <<USAGE
Usage: scripts/qlotto-netbsd.sh [lotto stress -Q args] [-- guest qemu args]

Arguments before -- are passed to Lotto.
Arguments after -- are appended to the guest QEMU command line.

Environment:
  LOTTO       Lotto binary path (default: build/lotto)
  SMOLBSD_DIR smolBSD directory (default: demos/smolBSD)
  SERVICE     smolBSD service/image name (default: lottoecho)
  ARCH        guest arch suffix (default: evbarm-aarch64)
  KERNEL      kernel image path
  IMAGE       root disk image path
USAGE
}

if [[ ${1:-} == "-h" || ${1:-} == "--help" ]]; then
    usage
    exit 0
fi

while [[ $# -gt 0 ]]; do
    case "$1" in
        --)
            shift
            QEMU_ARGS+=("$@")
            break
            ;;
        *)
            LOTTO_ARGS+=("$1")
            shift
            ;;
    esac
done

if [[ ! -x "$LOTTO" ]]; then
    echo "lotto binary not found/executable: $LOTTO" >&2
    exit 1
fi
if [[ ! -f "$KERNEL" ]]; then
    echo "kernel not found: $KERNEL" >&2
    exit 1
fi
if [[ ! -f "$IMAGE" ]]; then
    echo "image not found: $IMAGE" >&2
    exit 1
fi

exec "$LOTTO" stress -Q "${LOTTO_ARGS[@]}" -- \
    -kernel "$KERNEL" \
    -drive if=none,file="$IMAGE",format=raw,id=hd0 \
    -device virtio-blk-device,drive=hd0 \
    -append "console=com root=NAME=${SERVICE}root -z" \
    "${QEMU_ARGS[@]}"
