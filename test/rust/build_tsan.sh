#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
    echo "usage: $0 <crate-dir>" >&2
    exit 1
fi

crate_dir=$1

host=$(rustup run nightly rustc -vV | sed -n 's/^host: //p')
if [[ -z "$host" ]]; then
    echo "could not determine nightly rust host triple" >&2
    exit 1
fi

rustflags="${RUSTFLAGS:-} -Zsanitizer=thread -Zexternal-clangrt"
if [[ "$(uname -s)" == "Darwin" ]]; then
    rustflags+=" -C link-arg=-Wl,-undefined,dynamic_lookup"
fi
export RUSTFLAGS="$rustflags"

rustup run nightly cargo build \
    --manifest-path "$crate_dir/Cargo.toml" \
    --target "$host" \
    -Z build-std=std,panic_abort

bin="$crate_dir/target/$host/debug/tokio_lotto_replay"
if [[ ! -x "$bin" ]]; then
    echo "missing built binary: $bin" >&2
    exit 1
fi

printf '%s\n' "$bin" > "$crate_dir/binary.path"
