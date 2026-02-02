/*
 */

#include <lotto/engine/prng.h>
#include <lotto/qemu/lotto_qemu_syscall.h>

#define TRANSLATE_LIMIT (100)
#define YIELD_LIMIT     (50)

// TODO: fine tune this decision
// TODO: provide options via some flag
__attribute__((visibility("default"))) int
lotto_qemu_do_translate(uint64_t pc)
{
    /* Add sched_yield randomly */
    return prng_range(1, 100) <= TRANSLATE_LIMIT ? 1 : 0;
#if 0
    /* Always instrument */
    return 1;
#endif
}

// TODO: fine tune this decision
// TODO: provide options via some flag
__attribute__((visibility("default"))) int
lotto_qemu_do_yield(uint64_t pc)
{
    /* Call sched_yield randomly */
    return prng_range(1, 100) <= YIELD_LIMIT ? 1 : 0;
}

// TODO: provide this value as a flag
__attribute__((visibility("default"))) int
lotto_qemu_get_syscall_nr(void)
{
    return 124;
}
