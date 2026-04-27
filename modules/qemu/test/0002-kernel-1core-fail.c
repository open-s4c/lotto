// clang-format off
// REQUIRES: module-qemu
// RUN: (! %lotto %stress -Q -r 1 -- -smp 1 -kernel %builddir/modules/qemu/test/kernel/0002-kernel-1core-fail.elf 2>&1) | %check %s
// CHECK: fail: FAIL
// clang-format on
#include "kernel/kernel.h"
#include "kernel/uart.h"
#include <lotto/qemu/lotto_udf.h>

void
core_hook(unsigned int cpu_id)
{
    if (cpu_id != 0) {
        return;
    }

    uart_puts("fail: FAIL\r\n");
    LOTTO_EXIT_FAILURE;
}
