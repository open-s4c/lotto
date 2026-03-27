#!/bin/sh
set -eu

perl -0pi -e 's/ \(unnamed at [^)]*:(\d+):(\d+)\)/_$1_$2/g' "$@"
