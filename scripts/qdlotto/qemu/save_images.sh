#!/bin/bash
set -x
for image in images/qemu/*.qcow2; do
  cp -f ${image} ${image}.snap
done
for image in images/efi/*.qcow2; do
  cp -f ${image} ${image}.snap
done
