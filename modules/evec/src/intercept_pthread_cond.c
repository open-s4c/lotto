#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

#include <dice/chains/capture.h>
#include <dice/chains/intercept.h>
#include <dice/events/pthread.h>
#include <dice/events/thread.h>
#include <dice/interpose.h>
#include <dice/module.h>
#include <dice/pubsub.h>
#include <lotto/base/arg.h>
#include <lotto/base/category.h>
#include <lotto/base/context.h>
#include <lotto/evec.h>
#include <lotto/mutex.h>
#include <lotto/rsrc_deadlock.h>
#include <lotto/runtime/intercept.h>
#include <lotto/sys/logger.h>

static int
pthread_nop_zero_()
{
    return 0;
}

static int
pthread_nop_ETIMEDOUT_()
{
    return ETIMEDOUT;
}

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_COND_WAIT, {
    struct pthread_cond_wait_event *ev = EVENT_PAYLOAD(event);
    _lotto_evec_prepare(ev->cond);
    _lotto_mutex_release_named("pthread_cond_wait", ev->mutex);
    _lotto_evec_wait(ev->cond);
    _lotto_mutex_acquire_named("pthread_cond_wait", ev->mutex);
    ev->func = (void *)pthread_nop_zero_;
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_COND_TIMEDWAIT, {
    struct pthread_cond_timedwait_event *ev = EVENT_PAYLOAD(event);
    enum lotto_timed_wait_status ret;
    _lotto_evec_prepare(ev->cond);
    _lotto_mutex_release_named("pthread_cond_timedwait", ev->mutex);
    ret = _lotto_evec_timed_wait(ev->cond, ev->abstime);
    _lotto_mutex_acquire_named("pthread_cond_timedwait", ev->mutex);
    ev->func = (ret == TIMED_WAIT_TIMEOUT) ? (void *)pthread_nop_ETIMEDOUT_ :
        (void *)pthread_nop_zero_;
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_COND_SIGNAL, {
    struct pthread_cond_signal_event *ev = EVENT_PAYLOAD(event);
    _lotto_evec_wake(ev->cond, 1);
    ev->func = (void*)pthread_nop_zero_;
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_COND_BROADCAST, {
    struct pthread_cond_broadcast_event *ev = EVENT_PAYLOAD(event);
    _lotto_evec_wake(ev->cond, ~((uint32_t)0));
    ev->func = (void*)pthread_nop_zero_;
    return PS_OK;
})

#if 0
struct timespec64 {
    long long int tv_sec;
    long int tv_nsec;
};

int
___pthread_cond_clockwait64(pthread_cond_t *cond, pthread_mutex_t *mutex,
                            clockid_t clockid, const struct timespec64 *abstime)
{
    struct timespec ts = {.tv_sec  = (time_t)abstime->tv_sec,
                          .tv_nsec = abstime->tv_nsec};
    ASSERT(ts.tv_sec == abstime->tv_sec && "timespec overflow");
    return pthread_cond_timedwait(cond, mutex, &ts);
}
#endif
