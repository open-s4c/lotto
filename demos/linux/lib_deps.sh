#!/bin/bash

if [ $# -ne 1 ]; then
  echo "Usage: $0 <binary shared object>"
  exit 1
fi

CROSS="aarch64-linux-gnu"

READELF="${CROSS}-readelf"
LD="${CROSS}-ld"

SEARCH_DIRS=`${LD} --verbose | grep SEARCH_DIR | tr ';' '\n' | grep ${CROSS} | sed 's/^ //' | cut -d '=' -f 2 | cut -d '"' -f 1`
LIB_DIRS=""

RECURSIONS=3

FILE="$1"

DEPS=""
RESOLVED_DEPS=""

for dir in ${SEARCH_DIRS}; do
  if [ -d ${dir} ]; then
    LIB_DIRS="${dir} ${LIB_DIRS}"
  fi
done

function get_lib_deps {
  F=$1
  ${READELF} -d ${F} | grep 'Shared library' | cut -d '[' -f 2 | cut -d ']' -f 1
}

function lib_path {
  LIB=$1
  find ${LIB_DIRS} -name ${LIB} | head -n 1
}

#find ${LIB_DIRS} -name 'libc.so.6' | head -n 1

DEPS=`get_lib_deps ${FILE}`

for r in `seq 1 ${RECURSIONS}`; do
  for dep in ${DEPS}; do
    path=`lib_path "${dep}"`
    RESOLVED_DEPS="${path} ${RESOLVED_DEPS}"
    DEPS="`get_lib_deps ${path}` ${DEPS}"
  done
  DEPS="`echo ${DEPS} | tr ' ' '\n' | sort -u`"
done

RESOLVED_DEPS="`echo ${RESOLVED_DEPS} | tr ' ' '\n' | sort -u`"

echo "${RESOLVED_DEPS}"
