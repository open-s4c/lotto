/*
 */
#ifndef LOTTO_DROP_H
#define LOTTO_DROP_H
/**
 * @file drop.h
 * @brief The drop interface.
 *
 * The SUT can get into situations where a faulty behavior is
 * introduced by the verification process itself.  For example, if the
 * SUT is blocked inside a user-defined spin lock, other threads may
 * livelock. The drop interface provides a simple way to simply "drop"
 * the current execution if this happens.
 */

#include <stdbool.h>
#include <stdint.h>

void _lotto_drop() __attribute__((weak));

/**
 * Drop the current execution.
 */
static inline void
lotto_drop()
{
    if (_lotto_drop != NULL)
        _lotto_drop();
}

#endif
