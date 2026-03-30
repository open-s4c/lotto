#include <dice/chains/intercept.h>
#include <dice/module.h>
#include <dice/pubsub.h>
#include <lotto/engine/pubsub.h>
#include <lotto/evec.h>
#include <lotto/modules/evec/events.h>
#include <lotto/runtime/capture_point.h>
#include <lotto/sys/assert.h>

FORWARD_CAPTURE_TO_INGRESS(EVENT, EVENT_EVEC_PREPARE)
FORWARD_CAPTURE_TO_INGRESS(EVENT, EVENT_EVEC_WAIT)
FORWARD_CAPTURE_TO_INGRESS(EVENT, EVENT_EVEC_TIMED_WAIT)
FORWARD_CAPTURE_TO_INGRESS(EVENT, EVENT_EVEC_CANCEL)
FORWARD_CAPTURE_TO_INGRESS(EVENT, EVENT_EVEC_WAKE)
FORWARD_CAPTURE_TO_INGRESS(EVENT, EVENT_EVEC_MOVE)

void
intercept_evec_prepare(const char *func, void *addr, const void *pc)
{
    struct evec_prepare_event ev = {
        .pc   = pc,
        .addr = addr,
    };
    capture_point cp = {
        .chain_id = INTERCEPT_EVENT,
        .type_id  = EVENT_EVEC_PREPARE,
        .payload  = &ev,
        .func     = func,
    };
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_EVEC_PREPARE, &cp, 0);
}

void
intercept_evec_wait(const char *func, void *addr, const void *pc)
{
    struct evec_wait_event ev = {
        .pc   = pc,
        .addr = addr,
    };
    capture_point cp = {
        .chain_id = INTERCEPT_EVENT,
        .type_id  = EVENT_EVEC_WAIT,
        .payload  = &ev,
        .func     = func,
    };
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_EVEC_WAIT, &cp, 0);
}

enum lotto_timed_wait_status
intercept_evec_timed_wait(const char *func, void *addr,
                          const struct timespec *restrict abstime,
                          const void *pc)
{
    struct evec_timed_wait_event ev = {
        .pc      = pc,
        .addr    = addr,
        .abstime = abstime,
        .ret     = TIMED_WAIT_TIMEOUT,
    };
    capture_point cp = {
        .chain_id = INTERCEPT_EVENT,
        .type_id  = EVENT_EVEC_TIMED_WAIT,
        .payload  = &ev,
        .func     = func,
    };
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_EVEC_TIMED_WAIT, &cp, 0);
    return ev.ret;
}

void
intercept_evec_cancel(const char *func, void *addr, const void *pc)
{
    struct evec_cancel_event ev = {
        .pc   = pc,
        .addr = addr,
    };
    capture_point cp = {
        .chain_id = INTERCEPT_EVENT,
        .type_id  = EVENT_EVEC_CANCEL,
        .payload  = &ev,
        .func     = func,
    };
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_EVEC_CANCEL, &cp, 0);
}

void
intercept_evec_wake(const char *func, void *addr, uint32_t cnt, const void *pc)
{
    struct evec_wake_event ev = {
        .pc   = pc,
        .addr = addr,
        .cnt  = cnt,
    };
    capture_point cp = {
        .chain_id = INTERCEPT_EVENT,
        .type_id  = EVENT_EVEC_WAKE,
        .payload  = &ev,
        .func     = func,
    };
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_EVEC_WAKE, &cp, 0);
}

void
intercept_evec_move(const char *func, void *src, void *dst, const void *pc)
{
    struct evec_move_event ev = {
        .pc  = pc,
        .src = src,
        .dst = dst,
    };
    capture_point cp = {
        .chain_id = INTERCEPT_EVENT,
        .type_id  = EVENT_EVEC_MOVE,
        .payload  = &ev,
        .func     = func,
    };
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_EVEC_MOVE, &cp, 0);
}
