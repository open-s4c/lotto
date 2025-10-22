/* Minimal await-related stubs so integration tests can link. */
#include <stdint.h>

void
_lotto_await(void *addr)
{
    (void)addr;
}

void
__lotto_await(void *addr)
{
    (void)addr;
}

void
_lotto_spin_start(void)
{
}

void
_lotto_spin_end(uint32_t cond)
{
    (void)cond;
}

void
__lotto_spin_start(void)
{
}

void
__lotto_spin_end(uint32_t cond)
{
    (void)cond;
}
