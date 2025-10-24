#!/bin/bash

##### START ##### QDLOTTO SCRIPT HEADER #####
 : ${DIR_QDLOTTO:=`pwd`}

if [ "x$1" != "x" -a -d "$1" ]; then
  DIR_QDLOTTO="$1"
fi

DIR_QDLOTTO="`readlink -e ${DIR_QDLOTTO}`"
PATH_QDLOTTO_ENV="${DIR_QDLOTTO}/scripts/qdlotto/qdlotto.env"

if [ -f "${PATH_QDLOTTO_ENV}" ]; then
  source "${PATH_QDLOTTO_ENV}"
else
  DIR_INTER="`dirname $0`"
  DIR_INTER="`readlink -e ${DIR_INTER}`"
  DIR_QDLOTTO="${DIR_INTER%/*/*}"
  PATH_QDLOTTO_ENV="${DIR_QDLOTTO}/scripts/qdlotto/qdlotto.env"
  if [ -f "${PATH_QDLOTTO_ENV}" ]; then
    source "${PATH_QDLOTTO_ENV}"
  else
    echo "qdlotto.env not found at"
    echo "${PATH_QDLOTTO_ENV}"
    echo "Usage $0 [path/to/lotto-src]"
    exit 1
  fi
fi

DIR_CUR=`pwd`

if [ $? -ne 0 ]; then
  echo "Sourcing ${PATH_QDLOTTO_ENV} failed!"
  echo "Make sure to not execute from within scripts/"
  echo "Usage $0 [path/to/lotto-src]"
  exit 1
fi

##### END #### QDLOTTO SCRIPT HEADER #####

DIR_INIT="${DIR_CUR}"

if [ "${DIR_INIT}" != "${DIR_QDLOTTO}" ]; then
  ln -sf "${DIR_QDLOTTO}/scripts/qdlotto" "${DIR_INIT}/scripts"
fi

# from now on work from within DIR_INIT
cd "${DIR_INIT}"

if [ ! -d "${DIR_EFI}" ]; then
  mkdir -p "${DIR_EFI}"
fi
if [ ! -d "${DIR_QEMU}" ]; then
  mkdir -p "${DIR_QEMU}"
fi
if [ ! -d "${DIR_ALPINE}" ]; then
  mkdir -p "${DIR_ALPINE}"
fi
if [ ! -d "${DIR_MNT_LOOP}" ]; then
  mkdir -p "${DIR_MNT_LOOP}"
fi

# EFI
echo "Building EFI disk image."
URL_EFI="https://releases.linaro.org/components/kernel/uefi-linaro/16.02/release/qemu64/QEMU_EFI.fd"
FILE_EFI="`basename ${URL_EFI}`"
wget -q -c -O "${DIR_EFI}/${FILE_EFI}" "${URL_EFI}"
dd if=/dev/zero of="${DIR_EFI}/qemu_efi.img" bs=1M count=64 >/dev/null 2>&1
dd if="${DIR_EFI}/QEMU_EFI.fd" of="${DIR_EFI}/qemu_efi.img" conv=notrunc >/dev/null 2>&1
#qemu-img convert -f raw -O qcow2 "${DIR_EFI}/qemu_efi.img" "${DIR_EFI}/qemu_efi.qcow2"

# Alpine sys disk
echo "Building empty Alpine disk image."
dd if=/dev/zero of="${DIR_QEMU}/alpine.img" bs=1M count=60000 >/dev/null 2>&1
#qemu-img convert -f raw -O qcow2 "${DIR_QEMU}/alpine.img" "${DIR_QEMU}/alpine.qcow2"

echo "Downloading Alpine ISO."
URL_ALPINE_ISO="https://dl-cdn.alpinelinux.org/alpine/v3.20/releases/aarch64/alpine-standard-3.20.0-aarch64.iso"
FILE_ALPINE_ISO="`basename ${URL_ALPINE_ISO}`"
DIR_ALPINE_MNT="${DIR_QEMU}/mnt-alpine"
DIR_ALPINE_NEW="${DIR_QEMU}/new-alpine"
wget -q -c -O "${DIR_ALPINE}/${FILE_ALPINE_ISO}" "${URL_ALPINE_ISO}"

if [ -f "${DIR_ALPINE}/alpine.iso" ]; then
  echo "${DIR_ALPINE}/alpine.iso exists, not recreating."
else
  echo "Adding install scripts to Alpine ISO."
  mkdir -p "${DIR_ALPINE_MNT}"
  mkdir -p "${DIR_ALPINE_NEW}"

  sudo mount -t iso9660 -o ro,loop "${DIR_ALPINE}/${FILE_ALPINE_ISO}" "${DIR_ALPINE_MNT}"
  sudo cp -a "${DIR_ALPINE_MNT}/"* "${DIR_ALPINE_NEW}/"


  sudo cp -a scripts/alpine/* "${DIR_ALPINE_NEW}/"
  DIR_ABS_ALPINE="`readlink -e ${DIR_ALPINE}`"
  (cd ${DIR_ALPINE_NEW} && genisoimage -quiet -R -J -r -V LOTTO_ALPINE -c "boot.catalog" -e "boot/grub/efi.img" -o "${DIR_ABS_ALPINE}/alpine.iso" .)

  sudo umount "${DIR_ALPINE_MNT}"
  sudo rm -rf "${DIR_ALPINE_MNT}" "${DIR_ALPINE_NEW}"
  echo "New Alpine ISO created at ${DIR_ABS_ALPINE}/alpine.iso"
fi
