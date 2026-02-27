#include <errno.h>

#include "dice/pubsub.h"
#include "lotto/base/arg.h"
#include <dice/chains/capture.h>
#include <dice/events/pthread.h>
#include <dice/module.h>
#include <dice/self.h>
#include <lotto/base/context.h>
#include <lotto/order.h>
#include <lotto/runtime/intercept.h>
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

#define CASE_PTHREAD_JOIN_RET(VAL)                                             \
    case (VAL):                                                                \
        ev->func = (void *)pthread_nop_##VAL##_;                               \
        break


PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_THREAD_JOIN, {
    struct pthread_join_event *ev = EVENT_PAYLOAD(event);
    if (self_retired(md))
        return PS_STOP_CHAIN;
    ev->ret      = EINTR;
    context_t *c = ctx(.func = "pthread_join", .cat = CAT_JOIN,
                       .args = {
                           [0] = arg(uint64_t, ev->thread),
                           [1] = arg_ptr(ev->ptr),
                           [2] = arg_ptr(&ev->ret),
                       });
    intercept_capture(c);
    switch (ev->ret) {
        CASE_PTHREAD_JOIN_RET(EDEADLK);
        CASE_PTHREAD_JOIN_RET(EINVAL);
        CASE_PTHREAD_JOIN_RET(ESRCH);
        CASE_PTHREAD_JOIN_RET(0);
        default:
            logger_fatalf("Unexpected return value\n");
    }
    return PS_STOP_CHAIN;
})

PS_SUBSCRIBE(CAPTURE_AFTER, EVENT_THREAD_JOIN, {
    if (self_retired(md))
        return PS_STOP_CHAIN;
    //    logger_fatalf("This should not be called\n");
    return PS_STOP_CHAIN;
})
