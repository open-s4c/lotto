#include <stdint.h>

#include "kernel.h"
#include "uart.h"

/*
 * mailbox[i]  –  written by core 0 to wake secondary core i.
 *               Contains the address of secondary_entry().
 *               boot.S polls this and branches to the value when non-zero.
 *               Declared in BSS so it starts at zero.
 */
uint64_t mailbox[NUM_CORES];

/*
 * done[i]  –  set to 1 by core i after core_hook() returns.
 *             Core 0 polls this array to know when all cores have finished.
 */
volatile int done[NUM_CORES];

/* ------------------------------------------------------------------ */
extern void _start(void);

static inline int64_t
psci_cpu_on(uint64_t target_cpu, uint64_t entry, uint64_t context_id)
{
    register uint64_t x0 __asm__("x0") = 0x84000003UL; /* PSCI CPU_ON */
    register uint64_t x1 __asm__("x1") = target_cpu;
    register uint64_t x2 __asm__("x2") = entry;
    register uint64_t x3 __asm__("x3") = context_id;
    __asm__ volatile("hvc #0"
                     : "+r"(x0)
                     : "r"(x1), "r"(x2), "r"(x3)
                     : "memory");
    return (int64_t)x0;
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

static void
uart_core_marker(unsigned int cpu_id, const char *phase)
{
    uart_puts("kernel: core ");
    uart_putu(cpu_id);
    uart_putc(' ');
    uart_puts(phase);
    uart_puts("\r\n");
}

static void
uart_join_marker(unsigned int target_cpu, const char *phase)
{
    uart_puts("kernel: core 0 ");
    uart_puts(phase);
    uart_puts(" core ");
    uart_putu(target_cpu);
    uart_puts("\r\n");
}

/*
 * secondary_entry  –  called by boot.S for cores 1 .. NUM_CORES-1.
 *
 * boot.S branches here with x0 = cpu_id, so this matches the C calling
 * convention for a function taking one unsigned int argument.
 */
static void
secondary_entry(unsigned int cpu_id)
{
    uart_core_marker(cpu_id, "start");
    core_hook(cpu_id);
    uart_core_marker(cpu_id, "done");
    qlotto_yield();
    __atomic_store_n(&done[cpu_id], 1, __ATOMIC_RELEASE);
    qlotto_sev();
    /* Spin until the system exits. */
    for (;;)
        qlotto_wfe();
}

/* ------------------------------------------------------------------ */

/*
 * primary_main  –  called by boot.S for core 0 after BSS is cleared.
 */
void
primary_main(void)
{
    uart_puts("kernel: booting\r\n");
    pre_hook();

    /* Wake secondary cores by writing the entry point into their mailbox
     * slots.  A store-release followed by SEV ensures the secondaries see
     * the non-zero value and leave their spin loop.                        */
    for (unsigned int i = 1; i < NUM_CORES; i++) {
        __atomic_store_n(&mailbox[i], (uint64_t)secondary_entry,
                         __ATOMIC_RELEASE);
        (void)psci_cpu_on(i, (uint64_t)_start, i);
        qlotto_sev();
    }

    /* Run core 0's own test workload. */
    uart_core_marker(0, "start");
    core_hook(0);
    uart_core_marker(0, "done");
    __atomic_store_n(&done[0], 1, __ATOMIC_RELEASE);

    /* Wait for all cores to finish. */
    for (unsigned int i = 0; i < NUM_CORES; i++) {
        uart_join_marker(i, "wait");
        while (!__atomic_load_n(&done[i], __ATOMIC_ACQUIRE)) {
            qlotto_sev();
            qlotto_wfe();
        }
        uart_join_marker(i, "joined");
    }

    post_hook();
    uart_puts("kernel: all cores done\r\n");

    /*
     * Signal success.
     *
     * With the qlotto plugin loaded: the trap is intercepted and routed into
     * the Lotto termination module, which terminates the run cleanly.
     *
     * Without the qlotto plugin: the trap causes a synchronous exception.
     * The exception vector calls _exception_reset -> PSCI SYSTEM_RESET so
     * QEMU exits with -no-reboot.
     */
    LOTTO_EXIT_SUCCESS;

    /* Fallback: explicit PSCI SYSTEM_RESET in case the UDF was a no-op. */
    register uint64_t fn __asm__("x0") = 0x84000009UL;
    __asm__ volatile(
        "hvc #0\n\t"
        "smc #0\n\t" ::"r"(fn));
    for (;;)
        __asm__ volatile("wfi");
}

__attribute__((weak)) void
pre_hook(void)
{
}

__attribute__((weak)) void
post_hook(void)
{
}
