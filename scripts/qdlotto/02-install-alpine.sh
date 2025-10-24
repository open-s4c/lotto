#!/bin/bash

ALPIPE="/tmp/alpine-install-input"
ALPOUT="/tmp/alpine-install-output"

if [ -f ${ALPIPE} ]; then
  rm -f ${ALPIPE}
fi

if [ -f ${ALPOUT} ]; then
  rm -f ${ALPOUT}
fi

mkfifo ${ALPIPE}
sleep infinity > ${ALPIPE} &
PID_SLEEP_IN=$!

mkfifo ${ALPOUT}
sleep infinity > ${ALPOUT} &
PID_SLEEP_OUT=$!

scripts/qemu/qemu-alpine-install.sh < ${ALPIPE} | tee ${ALPOUT} &
PID_QEMU=$!

[ -p "${ALPOUT}" ]

commands_done=0
install_done=0
while [ ${install_done} -eq 0 ] ; do
  while read -r output; do
#    echo "$output"
    if [ ${commands_done} -eq 0 ]; then
      if [[ "$output" == *"(/dev/ttyAMA0)"*  ]]; then
        sleep 1
        echo "root" > ${ALPIPE}
        sleep 1
        echo "cd /media/vdb" > ${ALPIPE}
        sleep 1
        echo "./install_alpine.sh" > ${ALPIPE}
        commands_done=1
      fi
    fi
    if [[ "${output}" == *"Requesting system poweroff"* ]]; then
      install_done=1
      echo EOF > ${ALPIPE}
      echo EOF > ${ALPOUT}
      break
    fi
  done <"${ALPOUT}"
done

wait ${PID_QEMU}
kill -9 ${PID_SLEEP_IN}
kill -9 ${PID_SLEEP_OUT}

rm -f ${ALPIPE}
rm -f ${ALPOUT}

cp images/qemu/alpine.img images/qemu/alpine.img.clean
