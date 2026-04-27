#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LINUX_VERSION="${LINUX_VERSION:-6.13.4}"
MEM="${MEM:-200M}"
CPUS="${CPUS:-2}"
ROUNDS="${ROUNDS:-1}"
LOTTO="${LOTTO:-$ROOT_DIR/build/lotto}"
KERNEL="${KERNEL:-$ROOT_DIR/demos/linux/linux-${LINUX_VERSION}/arch/arm64/boot/Image}"
INITRD="${INITRD:-$ROOT_DIR/demos/linux/rootfs.img}"

LOTTO_ARGS=()
KERNEL_ARGS=()

usage() {
    cat <<USAGE
Usage: scripts/qlotto-linux.sh [lotto stress -Q args] [-- guest kernel args]

Arguments before -- are passed to Lotto.
Arguments after -- are appended to the guest kernel command line.

Environment:
  LOTTO           Lotto binary path (default: build/lotto)
  LINUX_VERSION   Linux version directory suffix (default: 6.13.4)
  MEM             guest memory (default: 200M)
  CPUS            guest CPU count (default: 2)
  ROUNDS          stress rounds (default: 1)
  KERNEL          kernel image path
  INITRD          initrd image path
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
            KERNEL_ARGS+=("$@")
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
if [[ ! -f "$INITRD" ]]; then
    echo "initrd not found: $INITRD" >&2
    exit 1
fi

exec "$LOTTO" stress -Q -r "$ROUNDS" \
    --qemu-mem "$MEM" \
    --qemu-cpu "$CPUS" \
    "${LOTTO_ARGS[@]}" -- \
    -kernel "$KERNEL" \
    -initrd "$INITRD" \
    -append "init=/init.sh serial=ttyAMA0 root=/dev/ram panic=-1 -- ${KERNEL_ARGS[*]}"
