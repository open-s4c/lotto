#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

#include <dice/chains/intercept.h>
#include <dice/events/pthread.h>
#include <dice/events/thread.h>
#include <dice/module.h>
#include <dice/pubsub.h>

#include <lotto/base/arg.h>
#include <lotto/base/category.h>
#include <lotto/base/context.h>
#include <lotto/runtime/intercept.h>
#include <lotto/rsrc_deadlock.h>

static inline context_t *
ctx_task_create(struct pthread_create_event *ev)
{
    context_t *c = ctx();
    c->func       = "pthread_create";
    c->cat        = CAT_TASK_CREATE;
    c->args[0]    = arg_ptr(ev->thread);
    c->args[1]    = arg_ptr(ev->attr);
    c->args[2]    = arg_ptr(ev->run);
    return c;
}

static inline context_t *
ctx_mutex(const char *func, pthread_mutex_t *mutex)
{
    context_t *c = ctx();
    c->func       = func;
    c->cat        = CAT_CALL;
    c->args[0]    = arg_ptr(mutex);
    return c;
}

static inline context_t *
ctx_cond(const char *func, pthread_mutex_t *mutex)
{
    context_t *c = ctx();
    c->func       = func;
    c->cat        = CAT_CALL;
    c->args[0]    = arg_ptr(mutex);
    return c;
}

static inline context_t *
ctx_join(struct pthread_join_event *ev)
{
    context_t *c = ctx();
    c->func       = "pthread_join";
    c->cat        = CAT_JOIN;
    c->args[0]    = arg_ptr(&ev->thread);
    c->args[1]    = arg_ptr(ev->ptr);
    c->args[2]    = arg_ptr(&ev->ret);
    return c;
}

static inline context_t *
ctx_thread_start(bool detached)
{
    context_t *c = ctx();
    c->func       = "pthread_thread_start";
    c->cat        = CAT_TASK_INIT;
    c->args[0]    = arg(uintptr_t, (uintptr_t)pthread_self());
    c->args[1]    = arg(bool, detached);
    return c;
}

static inline context_t *
ctx_thread_exit(struct pthread_exit_event *ev)
{
    context_t *c = ctx();
    c->func       = "pthread_exit";
    c->cat        = CAT_EXIT;
    c->args[0]    = arg_ptr(ev != NULL ? ev->ptr : NULL);
    return c;
}

PS_SUBSCRIBE(INTERCEPT_BEFORE, EVENT_THREAD_CREATE, {
    (void)chain;
    (void)type;
    (void)md;

    struct pthread_create_event *ev = EVENT_PAYLOAD(event);
    if (intercept_before_call != NULL) {
        (void)intercept_before_call(ctx_task_create(ev));
    }
    return PS_OK;
})

PS_SUBSCRIBE(INTERCEPT_AFTER, EVENT_THREAD_CREATE, {
    (void)chain;
    (void)type;
    (void)event;

    if (intercept_after_call != NULL) {
        intercept_after_call("pthread_create");
    }
    return PS_OK;
})

PS_SUBSCRIBE(INTERCEPT_EVENT, EVENT_THREAD_START, {
    (void)chain;
    (void)type;
    (void)event;
    (void)md;

    if (intercept_capture != NULL) {
        intercept_capture(ctx_thread_start(false));
    }
    return PS_OK;
})

PS_SUBSCRIBE(INTERCEPT_BEFORE, EVENT_THREAD_JOIN, {
    (void)chain;
    (void)type;

    struct pthread_join_event *ev = EVENT_PAYLOAD(event);
    ev->ret                       = EINTR;
    if (intercept_capture != NULL) {
        intercept_capture(ctx_join(ev));
    }
    return PS_OK;
})

PS_SUBSCRIBE(INTERCEPT_AFTER, EVENT_THREAD_JOIN, {
    (void)chain;
    (void)type;
    (void)event;

    if (intercept_after_call != NULL) {
        intercept_after_call("pthread_join");
    }
    return PS_OK;
})

PS_SUBSCRIBE(INTERCEPT_EVENT, EVENT_THREAD_EXIT, {
    (void)chain;
    (void)type;
    (void)md;

    struct pthread_exit_event *ev = EVENT_PAYLOAD(event);
    if (intercept_capture != NULL) {
        intercept_capture(ctx_thread_exit(ev));
    }
    return PS_OK;
})

PS_SUBSCRIBE(INTERCEPT_BEFORE, EVENT_MUTEX_LOCK, {
    (void)chain;
    (void)type;

    struct pthread_mutex_lock_event *ev = EVENT_PAYLOAD(event);
    lotto_rsrc_acquiring(ev->mutex);
    if (intercept_before_call != NULL) {
        (void)intercept_before_call(ctx_mutex("pthread_mutex_lock", ev->mutex));
    }
    return PS_OK;
})

PS_SUBSCRIBE(INTERCEPT_AFTER, EVENT_MUTEX_LOCK, {
    (void)chain;
    (void)type;
    (void)event;

    if (intercept_after_call != NULL) {
        intercept_after_call("pthread_mutex_lock");
    }
    return PS_OK;
})

#if !defined(__APPLE__)
PS_SUBSCRIBE(INTERCEPT_BEFORE, EVENT_MUTEX_TIMEDLOCK, {
    (void)chain;
    (void)type;

    struct pthread_mutex_timedlock_event *ev = EVENT_PAYLOAD(event);
    lotto_rsrc_acquiring(ev->mutex);
    if (intercept_before_call != NULL) {
        (void)intercept_before_call(
            ctx_mutex("pthread_mutex_timedlock", ev->mutex));
    }
    return PS_OK;
})

PS_SUBSCRIBE(INTERCEPT_AFTER, EVENT_MUTEX_TIMEDLOCK, {
    (void)chain;
    (void)type;
    (void)event;

    if (intercept_after_call != NULL) {
        intercept_after_call("pthread_mutex_timedlock");
    }
    return PS_OK;
})
#endif

PS_SUBSCRIBE(INTERCEPT_BEFORE, EVENT_MUTEX_TRYLOCK, {
    (void)chain;
    (void)type;
    (void)md;

    struct pthread_mutex_trylock_event *ev = EVENT_PAYLOAD(event);
    if (intercept_before_call != NULL) {
        (void)intercept_before_call(
            ctx_mutex("pthread_mutex_trylock", ev->mutex));
    }
    return PS_OK;
})

PS_SUBSCRIBE(INTERCEPT_AFTER, EVENT_MUTEX_TRYLOCK, {
    (void)chain;
    (void)type;
    (void)md;

    struct pthread_mutex_trylock_event *ev = EVENT_PAYLOAD(event);
    lotto_rsrc_tried_acquiring(ev->mutex, ev->ret == 0);
    if (intercept_after_call != NULL) {
        intercept_after_call("pthread_mutex_trylock");
    }
    return PS_OK;
})

PS_SUBSCRIBE(INTERCEPT_BEFORE, EVENT_MUTEX_UNLOCK, {
    (void)chain;
    (void)type;

    struct pthread_mutex_unlock_event *ev = EVENT_PAYLOAD(event);
    if (intercept_before_call != NULL) {
        (void)intercept_before_call(
            ctx_mutex("pthread_mutex_unlock", ev->mutex));
    }
    return PS_OK;
})

PS_SUBSCRIBE(INTERCEPT_AFTER, EVENT_MUTEX_UNLOCK, {
    (void)chain;
    (void)type;

    struct pthread_mutex_unlock_event *ev = EVENT_PAYLOAD(event);
    if (intercept_after_call != NULL) {
        intercept_after_call("pthread_mutex_unlock");
    }
    lotto_rsrc_released(ev->mutex);
    return PS_OK;
})

PS_SUBSCRIBE(INTERCEPT_BEFORE, EVENT_COND_WAIT, {
    (void)chain;
    (void)type;

    struct pthread_cond_wait_event *ev = EVENT_PAYLOAD(event);
    if (intercept_before_call != NULL) {
        (void)intercept_before_call(
            ctx_cond("pthread_cond_wait", ev->mutex));
    }
    return PS_OK;
})

PS_SUBSCRIBE(INTERCEPT_AFTER, EVENT_COND_WAIT, {
    (void)chain;
    (void)type;
    (void)event;

    if (intercept_after_call != NULL) {
        intercept_after_call("pthread_cond_wait");
    }
    return PS_OK;
})

PS_SUBSCRIBE(INTERCEPT_BEFORE, EVENT_COND_TIMEDWAIT, {
    (void)chain;
    (void)type;

    struct pthread_cond_timedwait_event *ev = EVENT_PAYLOAD(event);
    if (intercept_before_call != NULL) {
        (void)intercept_before_call(
            ctx_cond("pthread_cond_timedwait", ev->mutex));
    }
    return PS_OK;
})

PS_SUBSCRIBE(INTERCEPT_AFTER, EVENT_COND_TIMEDWAIT, {
    (void)chain;
    (void)type;
    (void)event;

    if (intercept_after_call != NULL) {
        intercept_after_call("pthread_cond_timedwait");
    }
    return PS_OK;
})

DICE_MODULE_INIT({})
