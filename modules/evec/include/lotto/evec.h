/**
 * @file evec.h
 * @brief Event counters for low level synchronization.
 *
 * Tasks can wait for a condition to be true and block if the condition is not
 * true. Otherwise, the task can cancel the waiting.
 */
#ifndef LOTTO_EVEC_H
#define LOTTO_EVEC_H

#include <stddef.h>
#include <stdint.h>
#include <time.h>

enum lotto_timed_wait_status {
    TIMED_WAIT_OK,
    TIMED_WAIT_TIMEOUT,
    TIMED_WAIT_INVALID
};

void intercept_evec_prepare(const char *func, void *addr, const void *pc)
    __attribute__((weak));
void intercept_evec_wait(const char *func, void *addr, const void *pc)
    __attribute__((weak));
enum lotto_timed_wait_status
intercept_evec_timed_wait(const char *func, void *addr,
                          const struct timespec *restrict abstime,
                          const void *pc)
    __attribute__((weak));
void intercept_evec_cancel(const char *func, void *addr, const void *pc)
    __attribute__((weak));
void intercept_evec_wake(const char *func, void *addr, uint32_t cnt,
                         const void *pc)
    __attribute__((weak));
void intercept_evec_move(const char *func, void *src, void *dst,
                         const void *pc)
    __attribute__((weak));

/**
 * Registers intent to wait for event.
 *
 * @param addr opaque event identifier
 */
static inline void
lotto_evec_prepare(void *addr)
{
    if (intercept_evec_prepare != NULL) {
        intercept_evec_prepare(__FUNCTION__, addr, __builtin_return_address(0));
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
    if (intercept_evec_wait != NULL) {
        intercept_evec_wait(__FUNCTION__, addr, __builtin_return_address(0));
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
    if (intercept_evec_timed_wait != NULL) {
        return intercept_evec_timed_wait(__FUNCTION__, addr, abstime,
                                         __builtin_return_address(0));
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
    if (intercept_evec_cancel != NULL) {
        intercept_evec_cancel(__FUNCTION__, addr, __builtin_return_address(0));
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
    if (intercept_evec_wake != NULL) {
        intercept_evec_wake(__FUNCTION__, addr, cnt,
                            __builtin_return_address(0));
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
    if (intercept_evec_move != NULL) {
        intercept_evec_move(__FUNCTION__, src, dst,
                            __builtin_return_address(0));
    }
}

#endif
