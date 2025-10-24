/*
 */
#ifndef LOTTO_EVEC_H
#define LOTTO_EVEC_H
/**
 * @file evec.h
 * @brief Event counters for low level synchronization.
 *
 * Tasks can wait for a condition to be true and block if the condition is not
 * true. Otherwise, the task can cancel the waiting.
 */

#include <stddef.h>
#include <stdint.h>
#include <time.h>

enum lotto_timed_wait_status {
    TIMED_WAIT_OK,
    TIMED_WAIT_TIMEOUT,
    TIMED_WAIT_INVALID
};

void _lotto_evec_prepare(void *addr) __attribute__((weak));
void _lotto_evec_wait(void *addr) __attribute__((weak));
enum lotto_timed_wait_status
_lotto_evec_timed_wait(void *addr, const struct timespec *restrict abstime)
    __attribute__((weak));
void _lotto_evec_cancel(void *addr) __attribute__((weak));
void _lotto_evec_wake(void *addr, uint32_t cnt) __attribute__((weak));
void _lotto_evec_move(void *src, void *dst) __attribute__((weak));

/**
 * Registers intent to wait for event.
 *
 * @param addr opaque event identifier
 */
static inline void
lotto_evec_prepare(void *addr)
{
    if (_lotto_evec_prepare != NULL) {
        _lotto_evec_prepare(addr);
    }
}

/**
 * Waits for event.
 *
 * @param addr opaque event identifier
 */
static inline void
lotto_evec_wait(void *addr)
{
    if (_lotto_evec_wait != NULL) {
        _lotto_evec_wait(addr);
    }
}

/**
 * Waits for event or the deadline.
 *
 * @param addr    opaque event identifier
 * @param abstime deadline timespec
 *
 * @return `TIMED_WAIT_OK` if returned by wake, `TIMED_WAIT_TIMEOUT` if returned
 * by timeout, `TIMED_WAIT_INVALID` if `abstime` is invalid
 */
static inline enum lotto_timed_wait_status
lotto_evec_timed_wait(void *addr, const struct timespec *restrict abstime)
{
    if (_lotto_evec_timed_wait != NULL) {
        return _lotto_evec_timed_wait(addr, abstime);
    }
    return TIMED_WAIT_TIMEOUT;
}

/**
 * Cancels intent to wait for event.
 *
 * @param addr opaque event identifier
 */
static inline void
lotto_evec_cancel(void *addr)
{
    if (_lotto_evec_cancel != NULL) {
        _lotto_evec_cancel(addr);
    }
}

/**
 * Wakes `cnt` waiters of the event.
 *
 * If there is no waiter, the wake is ignored.
 *
 * @param addr opaque event identifier
 * @param cnt maximum number of waiters to wake
 */
static inline void
lotto_evec_wake(void *addr, uint32_t cnt)
{
    if (_lotto_evec_wake != NULL) {
        _lotto_evec_wake(addr, cnt);
    }
}

/**
 * Move all waiters of `src` address to `dst address.
 *
 * @param src opaque event identifier from which all waiters will be moved.
 * @param dst opaque event identifier to which all waiters will be moved.
 */
static inline void
lotto_evec_move(void *src, void *dst)
{
    if (_lotto_evec_move != NULL) {
        _lotto_evec_move(src, dst);
    }
}

#endif
