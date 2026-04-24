// clang-format off
// REQUIRES: module-qemu
// RUN: %lotto %stress -Q -r 1 -- -smp 4 -kernel %builddir/modules/qemu/test/kernel/0005-kernel-4core-replay.elf | grep "replay_last_writer:" > %t.record
// RUN: test -s %t.record
// RUN: %lotto %replay | grep "replay_last_writer:" > %t.replay
// RUN: test -s %t.replay
// RUN: diff -u %t.record %t.replay
// clang-format on

#include "kernel/kernel.h"
#include "kernel/uart.h"

static volatile unsigned int replay_last_writer;

void
core_hook(unsigned int cpu_id)
{
    qlotto_yield();
    __atomic_store_n(&replay_last_writer, cpu_id, __ATOMIC_RELEASE);
}

void
post_hook(void)
{
    unsigned int v = __atomic_load_n(&replay_last_writer, __ATOMIC_ACQUIRE);
    uart_puts("replay_last_writer: ");
    uart_putu_local(v);
    uart_puts("\r\n");
}
