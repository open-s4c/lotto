#pragma once
#include <stdint.h>

/*
 * Minimal PL011 UART driver for QEMU virt.
 *
 * UART0 base address in QEMU virt: 0x09000000
 * DR  (data register)  : base + 0x00
 * FR  (flag register)  : base + 0x18
 *   TXFF bit (bit 5)   : TX FIFO full — wait before writing
 *
 * QEMU pre-initialises the UART; no setup is needed here.
 */

#define UART0_BASE   0x09000000UL
#define UART0_DR     ((volatile uint32_t *)(UART0_BASE + 0x00))
#define UART0_FR     ((volatile uint32_t *)(UART0_BASE + 0x18))
#define UART_FR_TXFF (1u << 5)

static inline void
uart_putc(char c)
{
    while (*UART0_FR & UART_FR_TXFF)
        ;
    *UART0_DR = (uint32_t)(unsigned char)c;
}

static inline void
uart_puts(const char *s)
{
    while (*s)
        uart_putc(*s++);
}

static inline void
uart_putu_local(unsigned int v)
{
    char buf[10];
    int n = 0;
    do {
        buf[n++] = (char)('0' + (v % 10));
        v /= 10;
    } while (v != 0);
    while (n > 0)
        uart_putc(buf[--n]);
}
