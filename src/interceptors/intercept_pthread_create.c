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
#include <lotto/rsrc_deadlock.h>
#include <lotto/runtime/intercept.h>
#include <lotto/sys/logger.h>

#if 0
int detachstate;
if (attr == NULL) {
    detachstate = PTHREAD_CREATE_JOINABLE;
} else {
    ENSURE(pthread_attr_getdetachstate(attr, &detachstate) == 0);
}
(void)intercept_capture(ctx(.func = __FUNCTION__, .cat = CAT_TASK_INIT,
                            .args = {arg(uint64_t, pthread_self()),
                                     arg(bool, detachstate ==
                                                   PTHREAD_CREATE_DETACHED)}));
void *ret = _run(_arg);
return ret;
}

#endif


// -----------------------------------------------------------------------------
// thread_start and thread_exit
// -----------------------------------------------------------------------------

PS_SUBSCRIBE(CAPTURE_EVENT, EVENT_THREAD_START, {
    bool detached = false;
    context_t *c  = ctx(.func = "pthread_thread_start", .cat = CAT_TASK_INIT,
                        .args = {
                            [0] = arg(uintptr_t, (uintptr_t)pthread_self()),
                            [1] = arg(bool, detached),
                       });
    intercept_capture(c);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_EVENT, EVENT_THREAD_EXIT, {
    struct pthread_exit_event *ev = EVENT_PAYLOAD(event);
    context_t *c = ctx(.func = "pthread_exit", .cat = CAT_TASK_FINI,
                       .args = {[0] = arg_ptr(ev != NULL ? ev->ptr : NULL)});
    intercept_capture(c);
    return PS_OK;
})

// -----------------------------------------------------------------------------
// pthread_create and pthread_join
// -----------------------------------------------------------------------------

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_THREAD_CREATE, {
    struct pthread_create_event *ev = EVENT_PAYLOAD(event);

    context_t *c = ctx(.func = "pthread_create", .cat = CAT_TASK_CREATE,
                       .args = {
                           [0] = arg_ptr(ev->thread),
                           [1] = arg_ptr(ev->attr),
                           [2] = arg_ptr(ev->run),
                       });
    (void)intercept_before_call(c);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_AFTER, EVENT_THREAD_CREATE, {
    intercept_after_call("pthread_create");
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_THREAD_JOIN, {
    struct pthread_join_event *ev = EVENT_PAYLOAD(event);

    ev->ret = EINTR;
    // alternative is CAT_JOIN
    context_t *c = ctx(.func = "pthread_join", .cat = CAT_CALL,
                       .args = {
                           [0] = arg_ptr(&ev->thread),
                           [1] = arg_ptr(ev->ptr),
                           [2] = arg_ptr(&ev->ret),
                       });
    (void)intercept_before_call(c);
})

PS_SUBSCRIBE(CAPTURE_AFTER, EVENT_THREAD_JOIN, {
    intercept_after_call("pthread_join");
    return PS_OK;
})


struct pthread_detach_event {
    const void *pc;
    pthread_t thread;
    void (*func)(pthread_t);
    int ret;
};

#define EVENT_PTHREAD_DETACH 7
PS_ADVERTISE_TYPE(EVENT_PTHREAD_DETACH)

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_PTHREAD_DETACH, {
    struct pthread_detach_event *ev = EVENT_PAYLOAD(event);

    int ret = EINTR;
    intercept_capture(ctx(.func = __FUNCTION__, .cat = CAT_DETACH,
                          .args = {
                              [0] = arg(uint64_t, ev->thread),
                              [1] = arg_ptr(&ret),
                          }));
    ASSERT(ret != EINTR);
    return PS_OK;
})

#if 0
INTERPOSE(int, pthread_detach, pthread_t thread)
{
    struct pthread_detach_event ev = {
        .pc     = INTERPOSE_PC,
        .thread = thread,
        .func   = REAL_FUNC(pthread_detach),
        .ret    = 0,
    };

    struct metadata md = {0};
    PS_PUBLISH(INTERCEPT_BEFORE, EVENT_PTHREAD_DETACH, &ev, &md);
    ev.ret = ev.func(ev.thread);
    PS_PUBLISH(INTERCEPT_AFTER, EVENT_PTHREAD_DETACH, &ev, &md);
    return ev.ret;
}
#endif
