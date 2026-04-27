// clang-format off
// REQUIRES: module-qemu
// REQUIRES: module-qemu_snapshot
// RUN: %lotto %stress -Q -r 1 -- -smp 2 -kernel %builddir/modules/qemu_snapshot/test/kernel/0001-kernel-2core-snapshot-race.elf | %check %s
// CHECK: snapshot-race: x=
// CHECK: kernel: all cores done
// clang-format on

#include <stdint.h>

#include <lotto/qemu/snapshot.h>

#include "kernel.h"
#include "uart.h"

static volatile uint32_t x;
static volatile uint32_t y;
static volatile uint32_t x_ready1;
static volatile uint32_t x_ready2;
static volatile uint32_t y_ready1;
static volatile uint32_t y_ready2;

static void
spin_delay(unsigned int iters)
{
    for (volatile unsigned int i = 0; i < iters; i++) {
    }
}

static void
uart_putu(unsigned int v)
{
    char buf[10];
    int n = 0;
    do {
        buf[n++] = (char)('0' + (v % 10));
        v /= 10;
    } while (v != 0);
    while (n > 0) {
        uart_putc(buf[--n]);
    }
}

void
pre_hook(void)
{
    x      = 0;
    y      = 0;
    x_ready1 = 0;
    x_ready2 = 0;
    y_ready1 = 0;
    y_ready2 = 0;
}

void
post_hook(void)
{
    uart_puts("snapshot-race: x=");
    uart_putu(x);
    uart_puts(" y=");
    uart_putu(y);
    uart_puts("\r\n");
}

void
core_hook(unsigned int cpu_id)
{
    if (cpu_id == 0) {
        x_ready1 = 1;
        qlotto_sev();

        while (x_ready2 == 0) {
            qlotto_wfe();
        }

        qlotto_snapshot();
        spin_delay(5000000);

        x = 1;

        y_ready1 = 1;
        qlotto_sev();

        while (y_ready2 == 0) {
            qlotto_wfe();
        }

        y = 1;
        spin_delay(5000000);
        return;
    }

    x_ready2 = 1;
    qlotto_sev();

    while (x_ready1 == 0) {
        qlotto_wfe();
    }

    x = 2;
    spin_delay(5000000);

    y_ready2 = 1;
    qlotto_sev();

    while (y_ready1 == 0) {
        qlotto_wfe();
    }

    y = 2;
    spin_delay(5000000);
}
