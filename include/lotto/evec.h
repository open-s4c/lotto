#ifndef LOTTO_EVEC_H
#define LOTTO_EVEC_H

#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include <dice/pubsub.h>

enum lotto_timed_wait_status {
    TIMED_WAIT_OK,
    TIMED_WAIT_TIMEOUT,
    TIMED_WAIT_INVALID
};

void _lotto_evec_prepare(void *addr, metadata_t *md) __attribute__((weak));
void _lotto_evec_wait(void *addr, metadata_t *md) __attribute__((weak));
enum lotto_timed_wait_status
_lotto_evec_timed_wait(void *addr, const struct timespec *restrict abstime,
                       metadata_t *md) __attribute__((weak));
void _lotto_evec_cancel(void *addr, metadata_t *md) __attribute__((weak));
void _lotto_evec_wake(void *addr, uint32_t cnt, metadata_t *md)
    __attribute__((weak));
void _lotto_evec_move(void *src, void *dst, metadata_t *md)
    __attribute__((weak));
#if 0
static inline void
lotto_evec_prepare(void *addr)
{
    if (_lotto_evec_prepare != NULL) {
        _lotto_evec_prepare(addr);
    }
}

static inline void
lotto_evec_wait(void *addr)
{
    if (_lotto_evec_wait != NULL) {
        _lotto_evec_wait(addr);
    }
}

static inline enum lotto_timed_wait_status
lotto_evec_timed_wait(void *addr, const struct timespec *restrict abstime)
{
    if (_lotto_evec_timed_wait != NULL) {
        return _lotto_evec_timed_wait(addr, abstime);
    }
    return TIMED_WAIT_TIMEOUT;
}

static inline void
lotto_evec_cancel(void *addr)
{
    if (_lotto_evec_cancel != NULL) {
        _lotto_evec_cancel(addr);
    }
}

static inline void
lotto_evec_wake(void *addr, uint32_t cnt)
{
    if (_lotto_evec_wake != NULL) {
        _lotto_evec_wake(addr, cnt);
    }
}

static inline void
lotto_evec_move(void *src, void *dst)
{
    if (_lotto_evec_move != NULL) {
        _lotto_evec_move(src, dst);
    }
}
#endif
#endif /* LOTTO_EVEC_H */
