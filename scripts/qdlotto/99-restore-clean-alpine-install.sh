#!/bin/bash

##### START ##### QDLOTTO SCRIPT HEADER #####
 : ${DIR_QDLOTTO:=`pwd`}

if [ "x$1" != "x" -a -d "$1" ]; then
  DIR_QDLOTTO="$1"
fi

DIR_QDLOTTO="`readlink -e ${DIR_QDLOTTO}`"
PATH_QDLOTTO_ENV="${DIR_QDLOTTO}/scripts/qdlotto.env"

if [ -f "${PATH_QDLOTTO_ENV}" ]; then
  source "${PATH_QDLOTTO_ENV}"
else
  DIR_INTER="`dirname $0`"
  DIR_INTER="`readlink -e ${DIR_INTER}`"
  DIR_QDLOTTO="${DIR_INTER%/*}"
  PATH_QDLOTTO_ENV="${DIR_QDLOTTO}/scripts/qdlotto.env"
  if [ -f "${PATH_QDLOTTO_ENV}" ]; then
    source "${PATH_QDLOTTO_ENV}"
  else
    echo "qdlotto.env not found at"
    echo "${PATH_QDLOTTO_ENV}"
#    echo "Usage $0 [path/to/qdlotto]"
    exit 1
  fi
fi

DIR_CUR=`pwd`

if [ $? -ne 0 ]; then
  echo "Sourcing ${PATH_QDLOTTO_ENV} failed!"
  echo "Make sure to not execute from within scripts/"
#  echo "Usage $0 [path/to/qdlotto]"
  exit 1
fi

##### END #### QDLOTTO SCRIPT HEADER #####

for file in `find . -name '*.qcow*'`; do
  echo "Removing file ${file}"
  rm -f "${file}"
done

for clean in `find . -name '*.img.clean'`; do
  img_name="${clean%.*}"
  echo "Reverting ${clean} to ${img_name}"
  cp "${clean}" "${img_name}"
done
