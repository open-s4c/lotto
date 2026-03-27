#!/bin/sh

set -eu

if [ -n "${CLANG_FORMAT:-}" ]; then
    FORMATTER="${CLANG_FORMAT}"
else
    FORMATTER="clang-format"
fi

if ! command -v "${FORMATTER}" >/dev/null 2>&1; then
    echo "clang-format not found: ${FORMATTER}" >&2
    exit 1
fi

supports_clang_format_() {
    case "$1" in
        *.c|*.cc|*.cpp|*.cxx|*.h|*.hh|*.hpp|*.hxx|*.inc|*.m|*.mm|*.proto)
            return 0
            ;;
        *)
            return 1
            ;;
    esac
}

tmp="$(mktemp)"
trap 'rm -f "${tmp}"' EXIT

{
    git diff --name-only --diff-filter=ACMRTUXB
    git diff --cached --name-only --diff-filter=ACMRTUXB
    git ls-files --others --exclude-standard
} | sort -u > "${tmp}"

formatted=0
while IFS= read -r path; do
    [ -n "${path}" ] || continue
    [ -f "${path}" ] || continue
    if ! supports_clang_format_ "${path}"; then
        continue
    fi
    "${FORMATTER}" -i -style=file "${path}"
    echo "formatted ${path}"
    formatted=1
done < "${tmp}"

if [ "${formatted}" -eq 0 ]; then
    echo "no modified clang-format-supported files"
fi
