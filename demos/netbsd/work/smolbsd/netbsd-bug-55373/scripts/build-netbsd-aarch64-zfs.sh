#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEFAULT_EXISTING_ROOT="/home/diogo/src/netbsd-release-11"
DEFAULT_FALLBACK_ROOT="/home/diogo/src/netbsd-11.0_RC3"
if [[ -z "${ROOT_DIR:-}" ]]; then
    if [[ -d "$DEFAULT_EXISTING_ROOT/src" ]]; then
        ROOT_DIR="$DEFAULT_EXISTING_ROOT"
    else
        ROOT_DIR="$DEFAULT_FALLBACK_ROOT"
    fi
fi
SRCROOT="${SRCROOT:-$ROOT_DIR/src}"
OBJDIR="${OBJDIR:-$ROOT_DIR/obj}"
JOBS="${JOBS:-$(nproc)}"
HOST_CFLAGS="${HOST_CFLAGS:--Wno-error=incompatible-pointer-types}"

mkdir -p "$ROOT_DIR"
cd "$ROOT_DIR"

if [[ ! -d "$SRCROOT" ]]; then
    echo "missing source tree: $SRCROOT" >&2
    echo "set ROOT_DIR to an existing NetBSD tree that already contains src/" >&2
    exit 1
fi

cd "$SRCROOT"

TOOLDIR="$(cd "$OBJDIR" 2>/dev/null && find . -maxdepth 1 -type d -name 'tooldir.*' | head -n 1 || true)"

if [[ -n "$TOOLDIR" && -x "$OBJDIR/${TOOLDIR#./}/bin/nbconfig" ]]; then
    echo "using existing tooldir: $OBJDIR/${TOOLDIR#./}"
else
    ./build.sh -U -O "$OBJDIR" -j"$JOBS" -m evbarm -a aarch64 \
        -V HOST_CFLAGS="$HOST_CFLAGS" -V MKINFO=no tools
fi

./build.sh -U -u -O "$OBJDIR" -j"$JOBS" -m evbarm -a aarch64 \
    -V HOST_CFLAGS="$HOST_CFLAGS" kernel=GENERIC64 modules

find "$OBJDIR" -name zfs.kmod -print
