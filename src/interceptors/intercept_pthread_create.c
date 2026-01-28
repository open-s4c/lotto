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

    ev->ret      = EINTR;
    context_t *c = ctx(.func = "pthread_join", .cat = CAT_CALL,
                       .args = {
                           [0] = arg_ptr(&ev->thread),
                           [1] = arg_ptr(ev->ptr),
                           [2] = arg_ptr(&ev->ret),
                       });
    (void)intercept_before_call(c);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_AFTER, EVENT_THREAD_JOIN, {
    intercept_after_call("pthread_join");
    return PS_OK;
})
