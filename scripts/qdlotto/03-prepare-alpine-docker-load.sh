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

if [ $# -lt 2 ]; then
  echo "Specify a docker loadable file as argument."
  echo "As well as a script to be called automatically upon boot of the Alpine qemu image."
  echo "Usage: $0 <docker-image.tar> <autorun.sh> [additional file or dir] ..."
  exit 1
fi

FILE_DOCKER_IMAGE="$1"
FILENAME_DOCKER_IMAGE="`basename ${FILE_DOCKER_IMAGE}`"
FILE_RUN_SCRIPT="$2"
FILENAME_RUN_SCRIPT="`basename ${FILE_RUN_SCRIPT}`"

echo "Building empty disk image."
dd if=/dev/zero of="${DIR_QEMU}/misc.img" bs=1M count=20000 >/dev/null 2>&1
sudo mkfs.ext4 "${DIR_QEMU}/misc.img" >/dev/null 2>&1
sudo mount -o loop "${DIR_QEMU}/misc.img" "${DIR_MNT_LOOP}"
sudo mkdir -p "${DIR_MNT_LOOP}/etc"

echo "Populate disk image."
sudo cp "${FILE_DOCKER_IMAGE}" "${DIR_MNT_LOOP}/docker-image.tar"
sudo cp "${FILE_RUN_SCRIPT}" "${DIR_MNT_LOOP}/run-script.sh"

EXTRA_FILES=0
if [ $# -gt 2 ]; then
  EXTRA_FILES=1
  # more files to copy to the image data/
  shift 2
  sudo mkdir "${DIR_MNT_LOOP}/data"
  sudo cp -a $@ "${DIR_MNT_LOOP}/data/"
fi

sudo rm -f "${DIR_MNT_LOOP}/etc/rc.local"

### copy scripts/alpine/configure_alpine.sh to etc/rc.local to be run after bootup in qemu Alpine
### It should
###   - install docker if not installed already
###   - make docker start automatically on boot
###   - start docker deamon right away
###   - remove all docker images if there are any
###   - load docker image from misc disk image
###   - add run script to Alpine image to be run on next boot automatically

sudo cp scripts/alpine/configure_alpine.sh "${DIR_MNT_LOOP}/etc/rc.local"
sudo chmod +x "${DIR_MNT_LOOP}/etc/rc.local"

# Add copying of files to etc/rc.local
if [ ${EXTRA_FILES} -eq 1 ]; then
  echo "mkdir /data" | sudo tee -a "${DIR_MNT_LOOP}/etc/rc.local" >/dev/null
  echo "cp -a data/* /data/" | sudo tee -a "${DIR_MNT_LOOP}/etc/rc.local" >/dev/null
fi

sudo umount "${DIR_MNT_LOOP}"

# echo "Convert disk image to qcow2"
# qemu-img convert -f raw -O qcow2 "${DIR_QEMU}/misc.img" "${DIR_QEMU}/misc.qcow2"

# echo "Created disk images ${DIR_QEMU}/misc.img ${DIR_QEMU}/misc.qcow2"
echo "Created disk images ${DIR_QEMU}/misc.img"
