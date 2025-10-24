/*
 */
#ifndef LOTTO_AWAIT_H
#define LOTTO_AWAIT_H
/**
 * @file await.h
 * @brief Await changes to a memory address
 */

#include <stddef.h>
#include <stdint.h>

void _lotto_await(void *addr) __attribute__((weak));

/**
 *
 * @brief Request lotto to suspend the current task until a write to `addr` is
 * detected.
 *
 * Execution may continue sporadically even if no write was detected, e.g. if no
 * tasks are ready. If the lotto `await` handler is not available or disabled,
 * this annotation will have no effect. The function call itself is non-blocking
 * and is just a hint to the scheduler to ignore this thread until a write is
 * detected.
 *
 * @param addr addr lotto should watch for writes
 */
static inline void
lotto_await(void *addr)
{
    if (_lotto_await != NULL) {
        _lotto_await(addr);
    }
}

#endif
