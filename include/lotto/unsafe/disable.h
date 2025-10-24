/*
 */
#ifndef LOTTO_UNSAFE_DISABLE_H
#define LOTTO_UNSAFE_DISABLE_H
/**
 * @file disable.h
 * @brief The interceptor disable interface.
 *
 * The disable interface allows one to prevent Lotto from receiving capture
 * points. Note that in this case the disabled region will be executed
 * atomically. This is an unsafe interface and the user is not recommended to
 * use it.
 */

#include <stdbool.h>
#include <stddef.h>

#define lotto_ghost(...)                                                       \
    {                                                                          \
        char ghost __attribute__((__cleanup__(lotto_ghost_cleanup)));          \
        (void)ghost;                                                           \
        lotto_disable();                                                       \
        __VA_ARGS__;                                                           \
    }

void _lotto_disable(void) __attribute__((weak));
void _lotto_enable(void) __attribute__((weak));
bool _lotto_disabled(void) __attribute__((weak));

/**
 * Disable the interceptor.
 */
static inline void
lotto_disable(void)
{
    if (_lotto_disable != NULL) {
        _lotto_disable();
    }
}

/**
 * Enable the interceptor.
 */
static inline void
lotto_enable(void)
{
    if (_lotto_enable != NULL) {
        _lotto_enable();
    }
}

/**
 * Returns whether the interceptor is disabled.
 */
static inline bool
lotto_disabled(void)
{
    return _lotto_disabled == NULL || _lotto_disabled();
}

static inline void __attribute__((unused)) lotto_ghost_cleanup(void *ghost)
{
    (void)ghost;
    lotto_enable();
}

#endif
