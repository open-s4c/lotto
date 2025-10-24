DEVICES="/dev/vda /dev/vdb"

for dev in ${DEVICES}; do
  mnt="/mnt/`basename ${dev}`"
  if [ -e ${dev} ]; then
    mkdir -p ${mnt}
    mount ${dev} ${mnt}
    if [ -f "${mnt}/etc/rc.local" ]; then
      chmod +x "${mnt}/etc/rc.local"
      (cd "${mnt}"; etc/rc.local)
    fi
  fi
done
