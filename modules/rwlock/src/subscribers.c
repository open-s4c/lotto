#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#include <dice/chains/capture.h>
#include <dice/events/pthread.h>
#include <dice/events/thread.h>
#include <dice/interpose.h>
#include <dice/module.h>
#include <dice/pubsub.h>
#include <lotto/base/arg.h>
#include <lotto/engine/pubsub.h>
#include <lotto/modules/rwlock/events.h>
#include <lotto/rsrc_deadlock.h>
#include <lotto/runtime/capture_point.h>
#include <lotto/sys/logger.h>

static int
const_zero(pthread_rwlock_t *l)
{
    return 0;
}
static int
const_zero_timed(pthread_rwlock_t *l, const struct timespec *t)
{
    return 0;
}

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_RWLOCK_RDLOCK, {
    struct pthread_rwlock_rdlock_event *ev = EVENT_PAYLOAD(event);
    struct rwlock_rdlock_event rev         = {
                .pc   = ev->pc,
                .func = "pthread_rwlock_rdlock",
                .lock = ev->lock,
    };
    capture_point cp = {
        .chain_id = chain,
        .type_id  = EVENT_RWLOCK_RDLOCK,
        .payload  = &rev,
        .func     = rev.func,
        .pc       = (uintptr_t)ev->pc,
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_RWLOCK_RDLOCK, &cp, md);
    ev->func = const_zero;
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_RWLOCK_WRLOCK, {
    struct pthread_rwlock_wrlock_event *ev = EVENT_PAYLOAD(event);
    struct rwlock_wrlock_event rev         = {
                .pc   = ev->pc,
                .func = "pthread_rwlock_wrlock",
                .lock = ev->lock,
    };
    capture_point cp = {
        .chain_id = chain,
        .type_id  = EVENT_RWLOCK_WRLOCK,
        .payload  = &rev,
        .func     = rev.func,
        .pc       = (uintptr_t)ev->pc,
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_RWLOCK_WRLOCK, &cp, md);
    ev->func = const_zero;
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_RWLOCK_UNLOCK, {
    struct pthread_rwlock_unlock_event *ev = EVENT_PAYLOAD(event);
    struct rwlock_unlock_event rev         = {
                .pc   = ev->pc,
                .func = "pthread_rwlock_unlock",
                .lock = ev->lock,
    };
    capture_point cp = {
        .chain_id = chain,
        .type_id  = EVENT_RWLOCK_UNLOCK,
        .payload  = &rev,
        .func     = rev.func,
        .pc       = (uintptr_t)ev->pc,
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_RWLOCK_UNLOCK, &cp, md);
    ev->func = const_zero;
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_RWLOCK_TRYRDLOCK, {
    struct pthread_rwlock_tryrdlock_event *ev = EVENT_PAYLOAD(event);
    ev->func                                  = const_zero;
    return PS_OK;
})
PS_SUBSCRIBE(CAPTURE_AFTER, EVENT_RWLOCK_TRYRDLOCK, {
    struct pthread_rwlock_tryrdlock_event *ev = EVENT_PAYLOAD(event);
    struct rwlock_tryrdlock_event rev         = {
                .pc   = ev->pc,
                .func = "pthread_rwlock_tryrdlock",
                .lock = ev->lock,
                .ret  = ev->ret,
    };
    capture_point cp = {
        .chain_id = chain,
        .type_id  = EVENT_RWLOCK_TRYRDLOCK,
        .payload  = &rev,
        .func     = rev.func,
        .pc       = (uintptr_t)ev->pc,
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_RWLOCK_TRYRDLOCK, &cp, md);
    ev->ret = rev.ret;
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_RWLOCK_TRYWRLOCK, {
    struct pthread_rwlock_trywrlock_event *ev = EVENT_PAYLOAD(event);
    ev->func                                  = const_zero;
    return PS_OK;
})
PS_SUBSCRIBE(CAPTURE_AFTER, EVENT_RWLOCK_TRYWRLOCK, {
    struct pthread_rwlock_trywrlock_event *ev = EVENT_PAYLOAD(event);
    struct rwlock_trywrlock_event rev         = {
                .pc   = ev->pc,
                .func = "pthread_rwlock_trywrlock",
                .lock = ev->lock,
                .ret  = ev->ret,
    };
    capture_point cp = {
        .chain_id = chain,
        .type_id  = EVENT_RWLOCK_TRYWRLOCK,
        .payload  = &rev,
        .func     = rev.func,
        .pc       = (uintptr_t)ev->pc,
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_RWLOCK_TRYWRLOCK, &cp, md);
    ev->ret = rev.ret;
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_RWLOCK_TIMEDRDLOCK, {
    struct pthread_rwlock_timedrdlock_event *ev = EVENT_PAYLOAD(event);
    struct rwlock_timedrdlock_event rev         = {
                .pc      = ev->pc,
                .func    = "pthread_rwlock_timedrdlock",
                .lock    = ev->lock,
                .abstime = ev->abstime,
    };
    capture_point cp = {
        .chain_id = chain,
        .type_id  = EVENT_RWLOCK_TIMEDRDLOCK,
        .payload  = &rev,
        .func     = rev.func,
        .pc       = (uintptr_t)ev->pc,
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_RWLOCK_TIMEDRDLOCK, &cp, md);
    ev->func = const_zero_timed;
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_RWLOCK_TIMEDWRLOCK, {
    struct pthread_rwlock_timedwrlock_event *ev = EVENT_PAYLOAD(event);
    struct rwlock_timedwrlock_event rev         = {
                .pc      = ev->pc,
                .func    = "pthread_rwlock_timedwrlock",
                .lock    = ev->lock,
                .abstime = ev->abstime,
    };
    capture_point cp = {
        .chain_id = chain,
        .type_id  = EVENT_RWLOCK_TIMEDWRLOCK,
        .payload  = &rev,
        .func     = rev.func,
        .pc       = (uintptr_t)ev->pc,
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_RWLOCK_TIMEDWRLOCK, &cp, md);
    ev->func = const_zero_timed;
    return PS_OK;
})
