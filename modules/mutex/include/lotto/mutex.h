/**
 * @file mutex.h
 * @brief Lotto-based mutex implementation
 *
 * Mutex provides no FIFO guarantees on the order of the waiters being awaken.
 */
#ifndef LOTTO_MUTEX_H
#define LOTTO_MUTEX_H

#include <stddef.h>

int intercept_mutex_tryacquire(const char *func, void *addr, const void *pc)
    __attribute__((weak));
void intercept_mutex_acquire(const char *func, void *addr, const void *pc)
    __attribute__((weak));
void intercept_mutex_release(const char *func, void *addr, const void *pc)
    __attribute__((weak));

/**
 * Acquires a mutex.
 *
 * @param addr opaque mutex identifier
 */
static inline void
lotto_mutex_acquire(void *addr, void *pc)
{
    if (intercept_mutex_acquire != NULL) {
        intercept_mutex_acquire(__FUNCTION__, addr, __builtin_return_address(0));
    }
}

/**
 * Tries to acquire a mutex.
 *
 * @param addr opaque mutex identifier
 * @return 0 on success, otherwise a non-zero value
 */
static inline int
lotto_mutex_tryacquire(void *addr)
{
    if (intercept_mutex_tryacquire != NULL) {
        return intercept_mutex_tryacquire(__FUNCTION__, addr,
                                          __builtin_return_address(0));
    }
    return 0;
}

/**
 * Releases mutex.
 *
 * @param addr opaque mutex identifier
 */
static inline void
lotto_mutex_release(void *addr, void *pc)
{
    if (intercept_mutex_release != NULL) {
        intercept_mutex_release(__FUNCTION__, addr, __builtin_return_address(0));
    }
}

#endif
