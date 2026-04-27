#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
SMOLBSD_DIR="$ROOT_DIR/demos/smolBSD"
SERVICE="${SERVICE:-lottoecho}"
ARCH="${ARCH:-evbarm-aarch64}"
IMAGE="${IMAGE:-$SMOLBSD_DIR/images/${SERVICE}-${ARCH}.img}"
KERNEL="${KERNEL:-$ROOT_DIR/demos/smolBSD/kernels/netbsd-GENERIC64.img}"
LOTTO="${LOTTO:-$ROOT_DIR/build/lotto}"
MAKE="${MAKE:-bmake}"
SKIP_BUILD="${SKIP_BUILD:-0}"
BUILD_TARGET="${BUILD_TARGET:-base}"
SMP="${SMP:-1}"
MEMACCESS="${MEMACCESS:-0}"
SAMPLING_CONFIG="${SAMPLING_CONFIG:-$SMOLBSD_DIR/sampling.conf}"

if [[ ! -f "$KERNEL" ]]; then
    echo "kernel not found: $KERNEL" >&2
    exit 1
fi
if [[ ! -x "$LOTTO" ]]; then
    echo "lotto binary not found/executable: $LOTTO" >&2
    exit 1
fi
if [[ ! -f "$SAMPLING_CONFIG" ]]; then
    echo "sampling config not found: $SAMPLING_CONFIG" >&2
    exit 1
fi
if [[ "$SKIP_BUILD" == "1" ]]; then
    echo "[1/2] Skipping build (SKIP_BUILD=1)"
else
    echo "[1/2] Building smolBSD image via SERVICE=${SERVICE} ARCH=${ARCH} TARGET=${BUILD_TARGET}"
    (
        cd "$SMOLBSD_DIR"
        "$MAKE" ARCH="$ARCH" SERVICE="$SERVICE" "$BUILD_TARGET"
    )
fi

if [[ ! -f "$IMAGE" ]]; then
    echo "service image not found: $IMAGE" >&2
    exit 1
fi

echo "[2/2] Running Lotto QEMU"
LOTTO_QEMU_INSTR_MEMACCESS="$MEMACCESS" exec "$LOTTO" stress -Q -r 1 \
    --sampling-config "$SAMPLING_CONFIG" -- \
    -m 256M -smp "$SMP" \
    -kernel "$KERNEL" \
    -drive if=none,file="$IMAGE",format=raw,id=hd0 \
    -device virtio-blk-device,drive=hd0 \
    -append "console=com root=NAME=${SERVICE}root -z"
