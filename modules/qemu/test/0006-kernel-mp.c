// clang-format off
// REQUIRES: module-qemu
// RUN: %lotto %stress -Q -r 1 -- -smp 4 -kernel %builddir/modules/qemu/test/kernel/0006-kernel-mp.elf | %check %s
// CHECK: mp: PASS
// clang-format on
/*
 * mp.c - ring message-passing test
 *
 * The four cores form a pipeline ring:
 *
 *   core 0  -->  core 1  -->  core 2  -->  core 3
 *      ^                                      |
 *      +--------------------------------------+
 *
 * Core 0 injects a counter value into slot[0]; each subsequent core reads
 * its input slot, clears it, and writes the value to its output slot.
 * After NUM_ROUNDS round trips core 0 verifies the final count and returns.
 *
 * Synchronisation uses C11 __atomic builtins for acquire/release ordering.
 * WFE/SEV reduce busy-wait overhead (QEMU TCG models them as no-ops, but
 * including them makes the test valid on real hardware too).
 *
 * This pattern exercises:
 *   - concurrent shared-memory reads and writes
 *   - producer/consumer ordering across cores
 *   - qlotto's memory-access instrumentation (loads, stores, atomics)
 */

#include "kernel/kernel.h"
#include "kernel/uart.h"

#define NUM_ROUNDS 20

/*
 * slot[i] carries a message from core i to core (i+1) % NUM_CORES.
 * Protocol: 0 = empty, non-zero = data.  The receiver clears the slot
 * before the sender may refill it, preventing overwrite races.
 */
static volatile unsigned int slot[NUM_CORES];

static void
print_turn_once(unsigned int cpu_id)
{
    uart_puts("mp: core ");
    uart_putu_local(cpu_id);
    uart_puts(" turn\r\n");
}

static void
send(unsigned int idx, unsigned int val)
{
    __atomic_store_n(&slot[idx], val, __ATOMIC_RELEASE);
    qlotto_sev();
}

static unsigned int
recv(unsigned int idx)
{
    unsigned int v;
    do {
        v = __atomic_load_n(&slot[idx], __ATOMIC_ACQUIRE);
        if (!v)
            qlotto_wfe();
    } while (!v);
    __atomic_store_n(&slot[idx], 0u, __ATOMIC_RELEASE);
    return v;
}

void
core_hook(unsigned int cpu_id)
{
    /*
     * Each core reads from slot[(cpu_id - 1 + N) % N] (its input)
     * and writes to slot[cpu_id] (its output).
     */
    unsigned int in_slot  = (cpu_id + NUM_CORES - 1u) % NUM_CORES;
    unsigned int out_slot = cpu_id;

    if (cpu_id == 0) {
        /*
         * Core 0 is the producer/verifier.
         * It injects round+1 into the ring and waits for the echo to
         * come back from core NUM_CORES-1.
         */
        print_turn_once(cpu_id);
        for (unsigned int r = 0; r < NUM_ROUNDS; r++) {
            send(out_slot, r + 1u);
            unsigned int echo = recv(in_slot);
            (void)echo;
        }
        uart_puts("mp: PASS\r\n");
    } else {
        /*
         * Forwarding cores: receive, then re-send to the next core.
         */
        bool printed = false;
        for (unsigned int r = 0; r < NUM_ROUNDS; r++) {
            unsigned int v = recv(in_slot);
            if (!printed) {
                print_turn_once(cpu_id);
                printed = true;
            }
            send(out_slot, v);
        }
    }
}
