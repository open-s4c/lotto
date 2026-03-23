#include <dice/chains/intercept.h>
#include <dice/module.h>
#include <dice/pubsub.h>
#include <lotto/engine/pubsub.h>
#include <lotto/modules/mutex/events.h>
#include <lotto/mutex.h>
#include <lotto/runtime/capture_point.h>
#include <lotto/sys/assert.h>

FORWARD_CAPTURE_TO_INGRESS(EVENT, EVENT_MUTEX_ACQUIRE)
FORWARD_CAPTURE_TO_INGRESS(EVENT, EVENT_MUTEX_TRYACQUIRE)
FORWARD_CAPTURE_TO_INGRESS(EVENT, EVENT_MUTEX_RELEASE)

void
intercept_mutex_acquire(const char *func, void *addr, const void *pc)
{
    struct mutex_acquire_event ev = {
        .pc   = pc,
        .func = func,
        .addr = addr,
    };
    capture_point cp = {
        .src_chain = INTERCEPT_EVENT,
        .src_type  = EVENT_MUTEX_ACQUIRE,
        .payload   = &ev,
        .func      = func,
        .pc        = (uintptr_t)pc,
    };
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_MUTEX_ACQUIRE, &cp, 0);
}

int
intercept_mutex_tryacquire(const char *func, void *addr, const void *pc)
{
    struct mutex_tryacquire_event ev = {
        .pc   = pc,
        .func = func,
        .addr = addr,
        .ret  = 0,
    };
    capture_point cp = {
        .src_chain = INTERCEPT_EVENT,
        .src_type  = EVENT_MUTEX_TRYACQUIRE,
        .payload   = &ev,
        .func      = func,
        .pc        = (uintptr_t)pc,
    };
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_MUTEX_TRYACQUIRE, &cp, 0);
    return ev.ret;
}

void
intercept_mutex_release(const char *func, void *addr, const void *pc)
{
    struct mutex_release_event ev = {
        .pc   = pc,
        .func = func,
        .addr = addr,
    };
    capture_point cp = {
        .src_chain = INTERCEPT_EVENT,
        .src_type  = EVENT_MUTEX_RELEASE,
        .payload   = &ev,
        .func      = func,
        .pc        = (uintptr_t)pc,
    };
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_MUTEX_RELEASE, &cp, 0);
}
