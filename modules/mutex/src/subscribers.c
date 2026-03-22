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
pthread_nop_one_()
{
    return 1;
}

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_PTHREAD_MUTEX_LOCK, {
    struct pthread_mutex_lock_event *ev = EVENT_PAYLOAD(event);

    struct mutex_acquire_event mev = {
        .pc   = ev->pc,
        .func = "pthread_mutex_lock",
        .addr = ev->mutex,
    };
    capture_point cp = {
        .src_chain = chain,
        .src_type  = EVENT_MUTEX_ACQUIRE,
        .payload   = &mev,
        .func      = mev.func,
        .pc        = (uintptr_t)ev->pc,
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_MUTEX_ACQUIRE, &cp, md);
    ev->func = (void *)pthread_nop_zero_;
    return PS_OK;
})


PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_PTHREAD_MUTEX_TRYLOCK, {
    struct pthread_mutex_trylock_event *ev = EVENT_PAYLOAD(event);

    struct mutex_tryacquire_event mev = {
        .pc   = ev->pc,
        .func = "pthread_mutex_trylock",
        .addr = ev->mutex,
        .ret  = 0,
    };
    capture_point cp = {
        .src_chain = chain,
        .src_type  = EVENT_MUTEX_TRYACQUIRE,
        .payload   = &mev,
        .func      = mev.func,
        .pc        = (uintptr_t)ev->pc,
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_MUTEX_TRYACQUIRE, &cp, md);
    ev->func =
        mev.ret == 0 ? (void *)pthread_nop_zero_ : (void *)pthread_nop_one_;
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_PTHREAD_MUTEX_UNLOCK, {
    struct pthread_mutex_unlock_event *ev = EVENT_PAYLOAD(event);

    struct mutex_release_event mev = {
        .pc   = ev->pc,
        .func = "pthread_mutex_unlock",
        .addr = ev->mutex,
    };
    capture_point cp = {
        .src_chain = chain,
        .src_type  = EVENT_MUTEX_RELEASE,
        .payload   = &mev,
        .func      = mev.func,
        .pc        = (uintptr_t)ev->pc,
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_MUTEX_RELEASE, &cp, md);
    ev->func = (void *)pthread_nop_zero_;
    return PS_OK;
})

#if !defined(__APPLE__)
PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_PTHREAD_MUTEX_TIMEDLOCK, {
    struct pthread_mutex_timedlock_event *ev = EVENT_PAYLOAD(event);

    struct mutex_acquire_event mev = {
        .pc   = ev->pc,
        .func = "pthread_mutex_timedlock",
        .addr = ev->mutex,
    };
    capture_point cp = {
        .src_chain = chain,
        .src_type  = EVENT_MUTEX_ACQUIRE,
        .payload   = &mev,
        .func      = mev.func,
        .pc        = (uintptr_t)ev->pc,
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_MUTEX_ACQUIRE, &cp, md);
    ev->func = (void *)pthread_nop_zero_;
    return PS_OK;
})
#endif
