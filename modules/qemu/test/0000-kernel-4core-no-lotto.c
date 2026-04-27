#include "kernel/kernel.h"
#include "kernel/uart.h"

void
core_hook(unsigned int cpu_id)
{
    if (cpu_id == 0) {
        uart_puts("no_lotto_4core: PASS\r\n");
    }
}
