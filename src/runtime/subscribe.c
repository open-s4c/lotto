#include <errno.h>
#include <pthread.h>
#include <stdint.h>

#ifdef DICE_MODULE_SLOT
    #undef DICE_MODULE_SLOT
#endif
#include <dice/chains/capture.h>
#include <dice/chains/intercept.h>
#include <dice/events/pthread.h>
#include <dice/events/self.h>
#include <dice/events/thread.h>
#include <dice/interpose.h>
#include <dice/module.h>
#include <dice/pubsub.h>
#include <dice/self.h>
#include <lotto/base/arg.h>
#include <lotto/engine/pubsub.h>
#include <lotto/runtime/capture_point.h>
#include <lotto/runtime/events.h>
#include <lotto/runtime/ingress_events.h>
#include <lotto/sys/logger.h>

// -----------------------------------------------------------------------------
// thread_start and thread_exit
// -----------------------------------------------------------------------------

PS_SUBSCRIBE(CAPTURE_EVENT, EVENT_SELF_INIT, {
    if (self_id(md) != MAIN_THREAD)
        return PS_OK;
    bool detached              = false;
    capture_task_init_event ev = {
        .thread   = (uintptr_t)pthread_self(),
        .detached = detached,
    };
    capture_point cp = {
        .src_chain = CAPTURE_EVENT,
        .src_type  = EVENT_TASK_INIT,
        .func      = "event_self_init",
        .task_init = &ev,
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_TASK_INIT, &cp, md);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_EVENT, EVENT_THREAD_START, {
    if (self_id(md) == MAIN_THREAD)
        return PS_OK;
    bool detached              = false;
    capture_task_init_event ev = {
        .thread   = (uintptr_t)pthread_self(),
        .detached = detached,
    };
    capture_point cp = {
        .src_chain = CAPTURE_EVENT,
        .src_type  = EVENT_TASK_INIT,
        .func      = "event_thread_start",
        .task_init = &ev,
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_TASK_INIT, &cp, md);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_EVENT, EVENT_THREAD_EXIT, {
    struct pthread_exit_event *ev = EVENT_PAYLOAD(ev);
    capture_task_fini_event fev   = {.ptr = ev != NULL ? ev->ptr : NULL};
    capture_point cp              = {
                     .src_chain = CAPTURE_EVENT,
                     .src_type  = EVENT_TASK_FINI,
                     .func      = "pthread_exit",
                     .task_fini = &fev,
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_TASK_FINI, &cp, md);
    return PS_OK;
})

// -----------------------------------------------------------------------------
// pthread_create
// -----------------------------------------------------------------------------

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_PTHREAD_CREATE, {
    struct pthread_create_event *ev = EVENT_PAYLOAD(ev);

    capture_task_create_event cev = {
        .thread = ev->thread,
        .attr   = ev->attr,
        .run    = ev->run,
    };
    capture_point cp = {
        .src_chain   = chain,
        .src_type    = EVENT_TASK_CREATE,
        .func        = "pthread_create",
        .task_create = &cev,
        .blocking    = true,
    };
    PS_PUBLISH(CHAIN_INGRESS_BEFORE, EVENT_TASK_CREATE, &cp, md);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_AFTER, EVENT_PTHREAD_CREATE, {
    struct pthread_create_event *ev = EVENT_PAYLOAD(ev);

    capture_task_create_event cev = {
        .thread = ev->thread,
        .attr   = ev->attr,
        .run    = ev->run,
    };
    capture_point cp = {
        .src_chain = chain,
        .src_type  = EVENT_TASK_CREATE,
        .func      = "pthread_create",
        .payload   = &cev,
        .blocking  = true,
    };
    PS_PUBLISH(CHAIN_INGRESS_AFTER, EVENT_TASK_CREATE, &cp, md);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_PTHREAD_JOIN, {
    struct pthread_join_event *ev = EVENT_PAYLOAD(ev);

    task_join_event_t tev = {
        .thread = ev->thread,
        .ptr    = ev->ptr,
        .ret    = EINTR,
    };
    capture_point cp = {
        .src_chain = chain,
        .src_type  = EVENT_TASK_JOIN,
        .func      = __FUNCTION__,
        .payload   = &tev,
        .blocking  = true,
    };
    PS_PUBLISH(CHAIN_INGRESS_BEFORE, EVENT_TASK_JOIN, &cp, md);
})

PS_SUBSCRIBE(CAPTURE_AFTER, EVENT_PTHREAD_JOIN, {
    struct pthread_join_event *ev = EVENT_PAYLOAD(ev);

    task_join_event_t tev = {
        .thread = ev->thread,
        .ptr    = ev->ptr,
        .ret    = ev->ret,
    };
    capture_point cp = {
        .src_chain = chain,
        .src_type  = EVENT_TASK_JOIN,
        .func      = __FUNCTION__,
        .payload   = &tev,
        .blocking  = true,
    };

    PS_PUBLISH(CHAIN_INGRESS_AFTER, EVENT_TASK_JOIN, &cp, md);
})


struct pthread_detach_event {
    const void *pc;
    pthread_t thread;
    void (*func)(pthread_t);
    int ret;
};

LOTTO_ADVERTISE_TYPE(EVENT_PTHREAD_DETACH)

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_PTHREAD_DETACH, {
    struct pthread_detach_event *ev = EVENT_PAYLOAD(event);

    int ret                       = EINTR;
    capture_task_detach_event dev = {
        .thread = ev->thread,
        .ret    = &ret,
    };
    capture_point cp = {
        .src_chain   = CAPTURE_BEFORE,
        .src_type    = EVENT_TASK_DETACH,
        .pc          = (uintptr_t)ev->pc,
        .func        = __FUNCTION__,
        .task_detach = &dev,
    };
    PS_PUBLISH(CHAIN_INGRESS_BEFORE, EVENT_TASK_DETACH, &cp, md);
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
