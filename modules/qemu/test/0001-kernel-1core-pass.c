// clang-format off
// REQUIRES: module-qemu
// RUN: %lotto %stress -Q -r 1 -- -smp 1 -kernel %builddir/modules/qemu/test/kernel/0001-kernel-1core-pass.elf | %check %s
// CHECK: run_end: PASS
// clang-format on
#include "kernel/kernel.h"
#include "kernel/uart.h"

void
core_hook(unsigned int cpu_id)
{
    if (cpu_id == 0) {
        uart_puts("run_end: PASS\r\n");
    }
}
