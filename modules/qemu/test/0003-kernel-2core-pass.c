// clang-format off
// REQUIRES: module-qemu
// RUN: %lotto %stress -Q -r 1 -- -smp 2 -kernel %builddir/modules/qemu/test/kernel/0003-kernel-2core-pass.elf | %check %s
// CHECK: kernel: all cores done
// clang-format on
#include <stdint.h>

#include "kernel/kernel.h"
#include "kernel/uart.h"

void
core_hook(unsigned int cpu_id)
{
    if (cpu_id == 0) {
        uart_puts("run_2cores_end: PASS\r\n");
    }
}
