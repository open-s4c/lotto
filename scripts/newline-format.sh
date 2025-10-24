#!/bin/bash

if [ $# -lt 1 ]; then
    echo "Usage: $0 [--apply] <PATH> [PATH ...]"
    echo ""
    echo "  PATH                one or more directories (or source files) to recursively run "
    echo "                      newline-format on."
    echo "Environment variables:"
    echo "  SILENT=true         disable git diff and error code"
    echo "  "
    echo ""
    exit 1
fi

if [ "${1}" = "--apply" ]; then
    APPLY="true"
    shift
fi

FOUND_MISSING=false

fix_newline() {
    check_newline "$1"
    result=$?
    if [ $result -ne 0 ]; then
        sed -i -e '$a\' "$1"
    fi
}

check_newline() {
    test "$(tail -c 1 "$1")"
    result=$?
    if [ $result -eq 0 ]; then
        FOUND_MISSING=true
        if [ "${SILENT}" != "true" ]; then
            echo "No newline in $1"
        else
            # As we dont output anything, exit right away if a missing newline is found
            exit 1
        fi
        return 1
    fi
    return 0
}

while IFS= read -r filename; do
    [ -e "${filename}" ] || continue # skip over truncated filenames due to newlines
    if [  "${APPLY}" = "true" ]; then
        fix_newline "${filename}"
    else
        check_newline "${filename}"
    fi
done < <(git ls-files --eol | grep 'w/lf' | tr -d '\t' | rev | cut -d ' ' -f 1 | rev | grep -v '^build/*' | grep -v '^deps/*' | grep -v '^mock/*')

if [ "${SILENT}" != "true" ] && [ "${APPLY}" = "true" ]; then
    # Display changed files
    git --no-pager diff
fi

if [ "${FOUND_MISSING}" = "true" ]; then
    # exit with 1 if there were differences.
    exit 1
fi
