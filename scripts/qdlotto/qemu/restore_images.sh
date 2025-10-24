#!/bin/bash

set -x
for image in images/qemu/*.snap; do
  [ "${image%.snap}" -nt ${image} ] && cp -f ${image} ${image%.snap}
done
for image in images/efi/*.snap; do
  [ "${image%.snap}" -nt ${image} ] && cp -f ${image} ${image%.snap}
done
