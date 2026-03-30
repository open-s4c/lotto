#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

#include <dice/chains/capture.h>
#include <dice/events/pthread.h>
#include <dice/events/thread.h>
#include <dice/interpose.h>
#include <dice/module.h>
#include <dice/pubsub.h>
#include <lotto/base/arg.h>
#include <lotto/engine/pubsub.h>
#include <lotto/evec.h>
#include <lotto/modules/evec/events.h>
#include <lotto/modules/mutex/events.h>
#include <lotto/mutex.h>
#include <lotto/rsrc_deadlock.h>
#include <lotto/runtime/capture_point.h>
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

static void
_publish_evec_prepare(const chain_id chain, struct metadata *md,
                      const char *func, void *addr, const void *pc)
{
    struct evec_prepare_event eev = {
        .pc   = pc,
        .addr = addr,
    };
    capture_point cp = {
        .chain_id = chain,
        .type_id  = EVENT_EVEC_PREPARE,
        .payload  = &eev,
        .func     = func,
        .pc       = (uintptr_t)pc,
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_EVEC_PREPARE, &cp, md);
}

static void
_publish_evec_wait(const chain_id chain, struct metadata *md, const char *func,
                   void *addr, const void *pc)
{
    struct evec_wait_event eev = {
        .pc   = pc,
        .addr = addr,
    };
    capture_point cp = {
        .chain_id = chain,
        .type_id  = EVENT_EVEC_WAIT,
        .payload  = &eev,
        .func     = func,
        .pc       = (uintptr_t)pc,
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_EVEC_WAIT, &cp, md);
}

static enum lotto_timed_wait_status
_publish_evec_timed_wait(const chain_id chain, struct metadata *md,
                         const char *func, void *addr,
                         const struct timespec *abstime, const void *pc)
{
    struct evec_timed_wait_event eev = {
        .pc      = pc,
        .addr    = addr,
        .abstime = abstime,
        .ret     = TIMED_WAIT_TIMEOUT,
    };
    capture_point cp = {
        .chain_id = chain,
        .type_id  = EVENT_EVEC_TIMED_WAIT,
        .payload  = &eev,
        .func     = func,
        .pc       = (uintptr_t)pc,
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_EVEC_TIMED_WAIT, &cp, md);
    return eev.ret;
}

static void
_publish_evec_wake(const chain_id chain, struct metadata *md, const char *func,
                   void *addr, uint32_t cnt, const void *pc)
{
    struct evec_wake_event eev = {
        .pc   = pc,
        .addr = addr,
        .cnt  = cnt,
    };
    capture_point cp = {
        .chain_id = chain,
        .type_id  = EVENT_EVEC_WAKE,
        .payload  = &eev,
        .func     = func,
        .pc       = (uintptr_t)pc,
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_EVEC_WAKE, &cp, md);
}

static void
_publish_mutex_release(const chain_id chain, struct metadata *md,
                       const char *func, void *addr, const void *pc)
{
    struct mutex_release_event mev = {
        .pc   = pc,
        .func = func,
        .addr = addr,
    };
    capture_point cp = {
        .chain_id = chain,
        .type_id  = EVENT_MUTEX_RELEASE,
        .payload  = &mev,
        .func     = mev.func,
        .pc       = (uintptr_t)pc,
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_MUTEX_RELEASE, &cp, md);
}

static void
_publish_mutex_acquire(const chain_id chain, struct metadata *md,
                       const char *func, void *addr, const void *pc)
{
    struct mutex_acquire_event mev = {
        .pc   = pc,
        .func = func,
        .addr = addr,
    };
    capture_point cp = {
        .chain_id = chain,
        .type_id  = EVENT_MUTEX_ACQUIRE,
        .payload  = &mev,
        .func     = mev.func,
        .pc       = (uintptr_t)pc,
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_MUTEX_ACQUIRE, &cp, md);
}

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_COND_WAIT, {
    struct pthread_cond_wait_event *ev = EVENT_PAYLOAD(event);
    _publish_evec_prepare(chain, md, "pthread_cond_wait", ev->cond, ev->pc);
    _publish_mutex_release(chain, md, "pthread_cond_wait", ev->mutex, ev->pc);
    _publish_evec_wait(chain, md, "pthread_cond_wait", ev->cond, ev->pc);
    _publish_mutex_acquire(chain, md, "pthread_cond_wait", ev->mutex, ev->pc);
    ev->func = (void *)pthread_nop_zero_;
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_COND_TIMEDWAIT, {
    struct pthread_cond_timedwait_event *ev = EVENT_PAYLOAD(event);
    enum lotto_timed_wait_status ret;
    _publish_evec_prepare(chain, md, "pthread_cond_timedwait", ev->cond,
                          ev->pc);
    _publish_mutex_release(chain, md, "pthread_cond_timedwait", ev->mutex,
                           ev->pc);
    ret = _publish_evec_timed_wait(chain, md, "pthread_cond_timedwait",
                                   ev->cond, ev->abstime, ev->pc);
    _publish_mutex_acquire(chain, md, "pthread_cond_timedwait", ev->mutex,
                           ev->pc);
    ev->func = (ret == TIMED_WAIT_TIMEOUT) ? (void *)pthread_nop_ETIMEDOUT_ :
                                             (void *)pthread_nop_zero_;
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_COND_SIGNAL, {
    struct pthread_cond_signal_event *ev = EVENT_PAYLOAD(event);
    _publish_evec_wake(chain, md, "pthread_cond_signal", ev->cond, 1, ev->pc);
    ev->func = (void *)pthread_nop_zero_;
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_COND_BROADCAST, {
    struct pthread_cond_broadcast_event *ev = EVENT_PAYLOAD(event);
    _publish_evec_wake(chain, md, "pthread_cond_broadcast", ev->cond,
                       ~((uint32_t)0), ev->pc);
    ev->func = (void *)pthread_nop_zero_;
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
