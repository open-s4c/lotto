#include <errno.h>

#include "dice/pubsub.h"
#include <dice/chains/capture.h>
#include <dice/events/pthread.h>
#include <dice/module.h>
#include <dice/self.h>
#include <lotto/engine/pubsub.h>
#include <lotto/modules/join/events.h>
#include <lotto/order.h>
#include <lotto/runtime/capture_point.h>
#include <lotto/runtime/ingress.h>
#include <lotto/runtime/ingress_events.h>
#include <lotto/sys/logger.h>

#define DECL_PTHREAD_JOIN_RET(VAL)                                             \
    static int pthread_nop_##VAL##_()                                          \
    {                                                                          \
        return (VAL);                                                          \
    }
DECL_PTHREAD_JOIN_RET(0)
DECL_PTHREAD_JOIN_RET(EDEADLK)
DECL_PTHREAD_JOIN_RET(EINVAL)
DECL_PTHREAD_JOIN_RET(ESRCH)
DECL_PTHREAD_JOIN_RET(EINTR)

#define CASE_PTHREAD_JOIN_RET(VAL)                                             \
    case (VAL):                                                                \
        ev->func = (void *)pthread_nop_##VAL##_;                               \
        break

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_PTHREAD_JOIN, {
    struct pthread_join_event *ev = EVENT_PAYLOAD(event);
    if (self_retired(md))
        return PS_STOP_CHAIN;

    task_join_event_t jev = {
        .thread = ev->thread,
        .ptr    = ev->ptr,
        .ret    = 0,
    };
    capture_point cp = {
        .src_chain = chain,
        .src_type  = EVENT_TASK_JOIN,
        .func      = "pthread_join",
        .payload   = &jev,
        .blocking  = false,
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_TASK_JOIN, &cp, md);
    switch (jev.ret) {
        CASE_PTHREAD_JOIN_RET(EDEADLK);
        CASE_PTHREAD_JOIN_RET(EINVAL);
        CASE_PTHREAD_JOIN_RET(ESRCH);
        CASE_PTHREAD_JOIN_RET(EINTR);
        CASE_PTHREAD_JOIN_RET(0);
        default:
            logger_fatalf("Unexpected return value\n");
    }
    return PS_STOP_CHAIN;
})

PS_SUBSCRIBE(CAPTURE_AFTER, EVENT_PTHREAD_JOIN, { return PS_STOP_CHAIN; })
