#!/bin/sh

set -e

if [ $# -lt 1 ]; then
    echo "Usage: $0 <PATH> [PATH ...]"
    echo ""
    echo "  PATH                one or more directories (or source files) to recursively run "
    echo "                      fmt on."
    echo "Environment variables:"
    echo "  SILENT=true         disable git diff and error code"
    echo "  "
    echo ""
    exit 1
fi

# Apply clang-format to all *.h and *.c files in src folder.
find "$@" \( -name '*.rkt' \) \
    -not -path '*/build/*' \
    -not -path '*/deps/*' \
    -not -path '*/mock/*' \
    -type f \
    -exec raco fmt -i {} +

if [ "${SILENT}" != "true" ]; then
    # Display changed files and exit with 1 if there were differences.
    git --no-pager diff --exit-code
fi
