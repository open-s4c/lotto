#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

// Adapts dice pthread publications into Lotto-style interceptor captures.

#include "interceptor.h"
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

DICE_MODULE_INIT({})

static inline void
ctx_task_create(context_t *c, struct pthread_create_event *ev, metadata_t *md)
{
    c->md      = md;
    c->func    = "pthread_create";
    c->cat     = CAT_TASK_CREATE;
    c->args[0] = arg_ptr(ev->thread);
    c->args[1] = arg_ptr(ev->attr);
    c->args[2] = arg_ptr(ev->run);
}

static inline void
ctx_mutex(context_t *c, const char *func, pthread_mutex_t *mutex,
          metadata_t *md)
{
    c->md      = md;
    c->func    = func;
    c->cat     = CAT_CALL;
    c->args[0] = arg_ptr(mutex);
}

static inline void
ctx_cond(context_t *c, const char *func, pthread_mutex_t *mutex, metadata_t *md)
{
    c->md      = md;
    c->func    = func;
    c->cat     = CAT_CALL;
    c->args[0] = arg_ptr(mutex);
}

static inline void
ctx_join(context_t *c, struct pthread_join_event *ev, metadata_t *md)
{
    c->md      = md;
    c->func    = "pthread_join";
    c->cat     = CAT_CALL;
    c->args[0] = arg_ptr(&ev->thread);
    c->args[1] = arg_ptr(ev->ptr);
    c->args[2] = arg_ptr(&ev->ret);
}

static inline void
ctx_thread_start(context_t *c, bool detached, metadata_t *md)
{
    c->md      = md;
    c->func    = "pthread_thread_start";
    c->cat     = CAT_TASK_INIT;
    c->args[0] = arg(uintptr_t, (uintptr_t)pthread_self());
    c->args[1] = arg(bool, detached);
}

static inline void
ctx_thread_exit(context_t *c, struct pthread_exit_event *ev, metadata_t *md)
{
    c->md      = md;
    c->func    = "pthread_exit";
    c->cat     = CAT_TASK_FINI;
    c->args[0] = arg_ptr(ev != NULL ? ev->ptr : NULL);
}

static inline void
ctx_sched_yield(context_t *c, metadata_t *md)
{
    c->md   = md;
    c->func = "sched_yield";
    c->cat  = CAT_USER_YIELD;
}


static inline void
send_after(const char *func, category_t cat, metadata_t *md)
{
    context_t *c = ctx();
    c->func      = func;
    c->cat       = cat;
    c->md        = md;
    intercept_after_call(c);
}

// -----------------------------------------------------------------------------
// thread_start and thread_exit
// -----------------------------------------------------------------------------

PS_SUBSCRIBE(CAPTURE_EVENT, EVENT_THREAD_START, {
    (void)chain;
    (void)type;
    (void)event;
    context_t *c = ctx();
    ctx_thread_start(c, false, md);
    intercept_capture(c);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_EVENT, EVENT_THREAD_EXIT, {
    (void)chain;
    (void)type;

    struct pthread_exit_event *ev = EVENT_PAYLOAD(event);
    context_t *c                  = ctx();
    ctx_thread_exit(c, ev, md);
    intercept_capture(c);
    return PS_OK;
})

// -----------------------------------------------------------------------------
// pthread_create and pthread_join
// -----------------------------------------------------------------------------

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_THREAD_CREATE, {
    (void)chain;
    (void)type;
    struct pthread_create_event *ev = EVENT_PAYLOAD(event);
    context_t *c                    = ctx();
    ctx_task_create(c, ev, md);
    (void)intercept_before_call(c);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_AFTER, EVENT_THREAD_CREATE, {
    (void)chain;
    (void)type;
    (void)event;

    send_after("pthread_create", CAT_TASK_CREATE, md);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_THREAD_JOIN, {
    (void)chain;
    (void)type;

    struct pthread_join_event *ev = EVENT_PAYLOAD(event);
    ev->ret                       = EINTR;
    context_t *c                  = ctx();
    ctx_join(c, ev, md);
    (void)intercept_before_call(c);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_AFTER, EVENT_THREAD_JOIN, {
    struct pthread_join_event *ev = EVENT_PAYLOAD(event);
    context_t *c                  = ctx();
    c->func                       = "pthread_join";
    c->cat                        = CAT_CALL;
    c->md                         = md;
    intercept_after_call(c);

    return PS_OK;
})

// -----------------------------------------------------------------------------
// pthread_mutex
// -----------------------------------------------------------------------------

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

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_MUTEX_LOCK, {
    struct pthread_mutex_lock_event *ev = EVENT_PAYLOAD(event);

    context_t *c = ctx();
    ctx_mutex(c, "pthread_mutex_lock", ev->mutex, md);
    c->cat = CAT_MUTEX_ACQUIRE;
    intercept_capture(c);

    ev->func = pthread_nop_zero_;
    return PS_OK;
})

#if !defined(__APPLE__)
PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_MUTEX_TIMEDLOCK, {
    (void)chain;
    (void)type;

    struct pthread_mutex_timedlock_event *ev = EVENT_PAYLOAD(event);
    lotto_rsrc_acquiring(ev->mutex);
    context_t *c = ctx();
    ctx_mutex(c, "pthread_mutex_timedlock", ev->mutex, md);
    (void)intercept_before_call(c);

    ev->func = pthread_nop_zero_;
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_AFTER, EVENT_MUTEX_TIMEDLOCK, {
    (void)chain;
    (void)type;
    (void)event;

    send_after("pthread_mutex_timedlock", CAT_CALL, md);
    return PS_OK;
})
#endif

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_MUTEX_TRYLOCK, {
    (void)chain;
    (void)type;

    struct pthread_mutex_trylock_event *ev = EVENT_PAYLOAD(event);
    context_t *c                           = ctx();
    ctx_mutex(c, "pthread_mutex_trylock", ev->mutex, md);
    (void)intercept_before_call(c);

    arg_t *ok = (arg_t *)&c->args[1];
    if (ok->value.u8 == 0)
        ev->func = pthread_nop_zero_;
    else
        ev->func = pthread_nop_one_;
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_AFTER, EVENT_MUTEX_TRYLOCK, {
    (void)chain;
    (void)type;

    struct pthread_mutex_trylock_event *ev = EVENT_PAYLOAD(event);
    lotto_rsrc_tried_acquiring(ev->mutex, ev->ret == 0);
    send_after("pthread_mutex_trylock", CAT_CALL, md);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_MUTEX_UNLOCK, {
    (void)chain;
    (void)type;
    struct pthread_mutex_unlock_event *ev = EVENT_PAYLOAD(event);

    context_t *c = ctx();
    ctx_mutex(c, "pthread_mutex_unlock", ev->mutex, md);
    c->cat = CAT_MUTEX_RELEASE;
    (void)intercept_capture(c);
    ev->func = pthread_nop_zero_;
    return PS_OK;
})

// -----------------------------------------------------------------------------
// pthread_cond
// -----------------------------------------------------------------------------

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_COND_WAIT, {
    (void)chain;
    (void)type;

    struct pthread_cond_wait_event *ev = EVENT_PAYLOAD(event);
    context_t *c                       = ctx();
    ctx_cond(c, "pthread_cond_wait", ev->mutex, md);
    (void)intercept_before_call(c);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_AFTER, EVENT_COND_WAIT, {
    (void)chain;
    (void)type;
    (void)event;

    send_after("pthread_cond_wait", CAT_CALL, md);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_COND_TIMEDWAIT, {
    (void)chain;
    (void)type;

    struct pthread_cond_timedwait_event *ev = EVENT_PAYLOAD(event);
    context_t *c                            = ctx();
    ctx_cond(c, "pthread_cond_timedwait", ev->mutex, md);
    (void)intercept_before_call(c);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_AFTER, EVENT_COND_TIMEDWAIT, {
    (void)chain;
    (void)type;
    (void)event;

    send_after("pthread_cond_timedwait", CAT_CALL, md);
    return PS_OK;
})

// -----------------------------------------------------------------------------
// sched_yield
// -----------------------------------------------------------------------------

PS_SUBSCRIBE(CAPTURE_EVENT, EVENT_SCHED_YIELD, {
    (void)chain;
    (void)type;
    (void)event;
    context_t *c = ctx();
    ctx_sched_yield(c, md);
    intercept_capture(c);
    return PS_OK;
})

PS_ADVERTISE_TYPE(EVENT_SCHED_YIELD)
INTERPOSE(int, sched_yield, void)
{
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_SCHED_YIELD, NULL, 0);
    return 0;
}

// -----------------------------------------------------------------------------
// memory accesses
//
// EVENT_MA_READ                 30       ./include/dice/events/memaccess.h
// EVENT_MA_WRITE                31       ./include/dice/events/memaccess.h
// EVENT_MA_AREAD                32       ./include/dice/events/memaccess.h
// EVENT_MA_AWRITE               33       ./include/dice/events/memaccess.h
// EVENT_MA_RMW                  34       ./include/dice/events/memaccess.h
// EVENT_MA_XCHG                 35       ./include/dice/events/memaccess.h
// EVENT_MA_CMPXCHG              36       ./include/dice/events/memaccess.h
// EVENT_MA_CMPXCHG_WEAK         37       ./include/dice/events/memaccess.h
// EVENT_MA_FENCE                38       ./include/dice/events/memaccess.h
// -----------------------------------------------------------------------------
#include <dice/events/memaccess.h>

PS_SUBSCRIBE(CAPTURE_EVENT, EVENT_MA_READ, {
    context_t *c = ctx();
    c->md        = md;
    c->cat       = CAT_BEFORE_READ;
    intercept_capture(c);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_EVENT, EVENT_MA_WRITE, {
    context_t *c = ctx();
    c->md        = md;
    c->cat       = CAT_BEFORE_WRITE;
    intercept_capture(c);
    return PS_OK;
})
