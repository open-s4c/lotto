/*
 */
#ifndef LOTTO_MUTEX_H
#define LOTTO_MUTEX_H
/**
 * @file mutex.h
 * @brief Lotto-based mutex implementation
 *
 * Mutex provides no FIFO guarantees on the order of the waiters being awaken.
 */

#include <stddef.h>

int _lotto_mutex_tryacquire(void *addr) __attribute__((weak));
int _lotto_mutex_tryacquire_named(const char *func, void *addr)
    __attribute__((weak));
void _lotto_mutex_acquire(void *addr) __attribute__((weak));
void _lotto_mutex_acquire_named(const char *func, void *addr)
    __attribute__((weak));
void _lotto_mutex_release(void *addr) __attribute__((weak));
void _lotto_mutex_release_named(const char *func, void *addr)
    __attribute__((weak));

/**
 * Acquires a mutex.
 *
 * @param addr opaque mutex identifier
 */
static inline void
lotto_mutex_acquire(void *addr)
{
    if (_lotto_mutex_acquire != NULL) {
        _lotto_mutex_acquire(addr);
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
    if (_lotto_mutex_tryacquire != NULL) {
        return _lotto_mutex_tryacquire(addr);
    }
    return 0;
}

/**
 * Releases mutex.
 *
 * @param addr opaque mutex identifier
 */
static inline void
lotto_mutex_release(void *addr)
{
    if (_lotto_mutex_release != NULL) {
        _lotto_mutex_release(addr);
    }
}

#endif
