/*
 * Deterministic bare-metal variant of inflex_abc:
 * two producer cores race a broken read/write ticket allocator and a third
 * core observes the duplicate id assignment.
 */
#include "kernel.h"
#include "uart.h"

static volatile unsigned int next_id;
static volatile unsigned int read_ready[2];
static volatile unsigned int produced[2];
static volatile unsigned int producer_id[2];
static volatile unsigned int recv_from[2];

static void
fail(void)
{
    uart_puts("gdb-abc: FAIL\r\n");
    LOTTO_EXIT_FAILURE;
    for (;;)
        qlotto_wfe();
}

void
pre_hook(void)
{
    next_id       = 0;
    read_ready[0] = 0;
    read_ready[1] = 0;
    produced[0]   = 0;
    produced[1]   = 0;
    recv_from[0]  = 0;
    recv_from[1]  = 0;
}

void
core_hook(unsigned int cpu_id)
{
    if (cpu_id < 2) {
        unsigned int id = __atomic_load_n(&next_id, __ATOMIC_RELAXED);

        read_ready[cpu_id] = 1u;
        qlotto_sev();
        while (read_ready[0] != 1u || read_ready[1] != 1u)
            qlotto_wfe();

        __atomic_store_n(&next_id, id + 1u, __ATOMIC_RELAXED);
        producer_id[cpu_id] = id;
        produced[cpu_id]    = 1u;
        qlotto_sev();
        return;
    }

    while (produced[0] != 1u || produced[1] != 1u)
        qlotto_wfe();

    for (unsigned int i = 0; i < 2; i++) {
        unsigned int id = producer_id[i];
        if (id < 2u)
            recv_from[id] = 1u;
    }

    if (recv_from[0] != 1u || recv_from[1] != 1u || next_id != 2u)
        fail();

    uart_puts("gdb-abc: PASS\r\n");
}
