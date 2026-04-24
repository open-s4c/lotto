#pragma once

#include <lotto/qemu/lotto_udf.h>

/* Number of vCPUs started by QEMU. Overridable per test target. */
#ifndef NUM_CORES
    #define NUM_CORES 4
#endif

/* Stack size per core (bytes). Must be a power of two ≥ 16. */
#define STACK_SIZE 8192

/*
 * core_hook - per-core test entry point
 *
 * Each core calls this function after booting.  The function should return
 * normally when the core's work is done; the kernel infrastructure then sets
 * done[cpu_id] and spins the core in WFE until the system exits.
 *
 * cpu_id: 0 .. NUM_CORES-1  (derived from MPIDR_EL1 Aff0)
 */
void pre_hook(void);
void core_hook(unsigned int cpu_id);
void post_hook(void);

static inline void
qlotto_yield(void)
{
    LOTTO_YIELD_A64;
}

static inline void
qlotto_sev(void)
{
    __asm__ volatile("sev");
    qlotto_yield();
}

static inline void
qlotto_wfe(void)
{
    qlotto_yield();
    __asm__ volatile("wfe");
}
