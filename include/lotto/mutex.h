#ifndef LOTTO_MUTEX_H
#define LOTTO_MUTEX_H

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

static inline void
lotto_mutex_acquire(void *addr)
{
    if (_lotto_mutex_acquire != NULL) {
        _lotto_mutex_acquire(addr);
    }
}

static inline int
lotto_mutex_tryacquire(void *addr)
{
    if (_lotto_mutex_tryacquire != NULL) {
        return _lotto_mutex_tryacquire(addr);
    }
    return 0;
}

static inline void
lotto_mutex_release(void *addr)
{
    if (_lotto_mutex_release != NULL) {
        _lotto_mutex_release(addr);
    }
}

#endif /* LOTTO_MUTEX_H */
