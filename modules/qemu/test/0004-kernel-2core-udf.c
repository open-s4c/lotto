// clang-format off
// REQUIRES: module-qemu
// RUN: (! %lotto %stress -Q -r 1 -- -smp 2 -kernel %builddir/modules/qemu/test/kernel/0004-kernel-2core-udf.elf > /dev/null 2>&1)
// clang-format on
#include <stdint.h>

#include "kernel/kernel.h"
#include "kernel/uart.h"

static volatile uint32_t ready0;
static volatile uint32_t ready1;

void
core_hook(unsigned int cpu_id)
{
    if (cpu_id == 0) {
        __atomic_store_n(&ready0, 1u, __ATOMIC_RELEASE);
    } else if (cpu_id == 1) {
        __atomic_store_n(&ready1, 1u, __ATOMIC_RELEASE);
    }
    qlotto_sev();

    while (!(__atomic_load_n(&ready0, __ATOMIC_ACQUIRE) &&
             __atomic_load_n(&ready1, __ATOMIC_ACQUIRE))) {
        qlotto_wfe();
    }

    if (cpu_id == 0) {
        uart_puts("mixed_udf: core0 FAIL\r\n");
        LOTTO_EXIT_FAILURE;
    } else if (cpu_id == 1) {
        uart_puts("mixed_udf: core1 STOP\r\n");
        LOTTO_EXIT_ABANDON;
    }
}
