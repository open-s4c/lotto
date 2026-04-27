#!/usr/bin/env bash
set -euo pipefail

NETBSD_DEMO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ROOT_DIR="$(cd "$NETBSD_DEMO_DIR/../.." && pwd)"
SMOLBSD_DIR="$ROOT_DIR/demos/smolBSD"
WORK_ROOT="$NETBSD_DEMO_DIR/work"

ARCH="${ARCH:-evbarm-aarch64}"
MAKE="${MAKE:-bmake}"
BUILD_TARGET="${BUILD_TARGET:-build}"
BUILDNONET="${BUILDNONET:-y}"
QEMU="${QEMU:-/usr/bin/qemu-system-aarch64}"

netbsd_require_tools()
{
    command -v "$MAKE" >/dev/null
    command -v "$QEMU" >/dev/null
}

netbsd_service_dir()
{
    printf '%s\n' "$WORK_ROOT/services/$1"
}

netbsd_workspace_dir()
{
    printf '%s\n' "$WORK_ROOT/smolbsd/$1"
}

netbsd_image_path()
{
    printf '%s\n' "$SMOLBSD_DIR/images/$1-$ARCH.img"
}

netbsd_reset_service_dir()
{
    local service_dir
    service_dir="$(netbsd_service_dir "$1")"
    rm -rf "$service_dir"
    mkdir -p "$service_dir/etc" "$service_dir/postinst"
    printf '%s\n' "$service_dir"
}

netbsd_prepare_workspace()
{
    local service="$1"
    local service_dir workspace
    local build_img

    service_dir="$(netbsd_service_dir "$service")"
    workspace="$(netbsd_workspace_dir "$service")"
    build_img="build-$ARCH.img"

    rm -rf "$workspace"
    mkdir -p "$workspace/service" "$workspace/tmp" "$workspace/scripts" \
        "$workspace/service/common" "$workspace/sets/$ARCH" \
        "$workspace/pkgs/$ARCH" "$workspace/kernels" "$workspace/images" \
        "$workspace/bios"

    cp "$SMOLBSD_DIR/Makefile" "$workspace/Makefile"
    cp "$SMOLBSD_DIR/mkimg.sh" "$workspace/mkimg.sh"
    cp "$SMOLBSD_DIR/startnb.sh" "$workspace/startnb.sh"
    cp -a "$SMOLBSD_DIR/scripts/." "$workspace/scripts/"
    cp -a "$SMOLBSD_DIR/service/common/." "$workspace/service/common/"
    cp -a "$service_dir" "$workspace/service/$service"
    cp -a "$SMOLBSD_DIR/sets/$ARCH/." "$workspace/sets/$ARCH/"
    if [ -d "$SMOLBSD_DIR/pkgs/$ARCH" ]; then
        cp -a "$SMOLBSD_DIR/pkgs/$ARCH/." "$workspace/pkgs/$ARCH/"
    fi
    cp -a "$SMOLBSD_DIR/kernels/." "$workspace/kernels/"
    cp -a "$SMOLBSD_DIR/bios/." "$workspace/bios/"
    if [ -f "$SMOLBSD_DIR/images/$build_img" ]; then
        cp "$SMOLBSD_DIR/images/$build_img" "$workspace/images/$build_img"
    fi
}

netbsd_build_service()
{
    local service="$1"
    local workspace
    local built_image
    local built_sig

    workspace="$(netbsd_workspace_dir "$service")"
    netbsd_prepare_workspace "$service"
    built_image="$workspace/images/$service-$ARCH.img"
    built_sig="$workspace/images/$service-$ARCH.sig"

    (
        cd "$workspace"
        env QEMU="$QEMU" \
            "$MAKE" \
            "BUILDNONET=$BUILDNONET" \
            "ARCH=$ARCH" \
            "SERVICE=$service" \
            "$BUILD_TARGET"
    )

    if [ -f "$built_image" ]; then
        mkdir -p "$SMOLBSD_DIR/images"
        cp "$built_image" "$SMOLBSD_DIR/images/"
    fi
    if [ -f "$built_sig" ]; then
        mkdir -p "$SMOLBSD_DIR/images"
        cp "$built_sig" "$SMOLBSD_DIR/images/"
    fi
}

netbsd_write_qlotto_sources()
{
    local service_dir="$1"
    cp "$NETBSD_DEMO_DIR/lib/qlotto_guest.h" "$service_dir/qlotto_guest.h"
    cp "$NETBSD_DEMO_DIR/lib/qlotto_run_command.c" \
        "$service_dir/qlotto_run_command.c"
}

netbsd_print_artifact_summary()
{
    local service="$1"
    printf 'service=%s\n' "$service"
    printf 'image=%s\n' "$(netbsd_image_path "$service")"
}
