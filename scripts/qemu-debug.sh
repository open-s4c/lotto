#!/bin/bash

pid="$(ps ax | grep qemu-system-aarch | grep -v grep | tail -n1)"
pid="$(echo $pid | cut -d' ' -f 1)"
exec sudo gdb --pid=$pid
