// clang-format off
// REQUIRES: module-qemu
// RUN: sh -c '%lotto %stress -Q -r 1 -- -smp 4 -kernel %builddir/modules/qemu/test/kernel/0007-kernel-4core-terminal-race.elf >/dev/null 2>&1; echo $? > %t.record'
// RUN: sh -c '%lotto %replay >/dev/null 2>&1; echo $? > %t.replay'
// RUN: diff -u %t.record %t.replay
// clang-format on

#include <stdint.h>

#include "kernel/kernel.h"
#include "kernel/uart.h"

static volatile uint32_t ready_fail;
static volatile uint32_t ready_stop;

void
core_hook(unsigned int cpu_id)
{
    if (cpu_id == 0) {
        ready_fail = 1u;
    } else if (cpu_id == 1) {
        ready_stop = 1u;
    } else {
        qlotto_yield();
        return;
    }
    qlotto_sev();

    while (!(ready_fail && ready_stop)) {
        qlotto_wfe();
    }

    qlotto_yield();
    if (cpu_id == 0) {
        uart_puts("winner: core0 FAIL\r\n");
        LOTTO_EXIT_FAIL;
    } else {
        uart_puts("winner: core1 STOP\r\n");
        LOTTO_EXIT_STOP;
    }
}
