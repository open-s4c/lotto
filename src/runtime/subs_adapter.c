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

static int
pthread_nop_ETIMEDOUT_()
{
    return ETIMEDOUT;
}

int
_lotto_mutex_lock(const char *func, void *mutex, metadata_t *md)
{
    context_t *c = ctx();
    ctx_mutex(c, func, mutex, md);
    c->cat = CAT_MUTEX_ACQUIRE;
    (void)intercept_capture(c);
    return 0;
}

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_MUTEX_LOCK, {
    struct pthread_mutex_lock_event *ev = EVENT_PAYLOAD(event);
    (void)_lotto_mutex_lock("pthread_mutex_lock", ev->mutex, md);
    ev->func = pthread_nop_zero_;
    return PS_OK;
})

int
_lotto_mutex_trylock(const char *func, void *mutex, metadata_t *md)
{
    context_t *c = ctx();
    ctx_mutex(c, func, mutex, md);
    c->cat = CAT_MUTEX_TRYACQUIRE;
    (void)intercept_capture(c);
    arg_t *ok = (arg_t *)&c->args[1];
    return ok->value.u8;
}

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_MUTEX_TRYLOCK, {
    struct pthread_mutex_trylock_event *ev = EVENT_PAYLOAD(event);

    ev->func =
        _lotto_mutex_trylock("pthread_mutex_trylock", ev->mutex, md) == 0 ?
            pthread_nop_zero_ :
            pthread_nop_one_;
    return PS_OK;
})

int
_lotto_mutex_unlock(const char *func, void *mutex, metadata_t *md)
{
    context_t *c = ctx();
    ctx_mutex(c, func, mutex, md);
    c->cat = CAT_MUTEX_RELEASE;
    (void)intercept_capture(c);
    return 0;
}

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_MUTEX_UNLOCK, {
    struct pthread_mutex_unlock_event *ev = EVENT_PAYLOAD(event);
    (void)_lotto_mutex_unlock("pthread_mutex_unlock", ev->mutex, md);
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

#endif

// -----------------------------------------------------------------------------
// pthread_cond
// -----------------------------------------------------------------------------
#include <lotto/evec.h>

void
_lotto_evec_prepare(void *addr, metadata_t *md)
{
    intercept_capture(
        ctx(.func = __FUNCTION__, .cat = CAT_SYS_YIELD, .md = md));
    intercept_capture(ctx(.func = __FUNCTION__, .cat = CAT_EVEC_PREPARE,
                          .md = md, .args = {arg_ptr(addr)}));
}

void
_lotto_evec_wait(void *addr, metadata_t *md)
{
    intercept_capture(
        ctx(.func = __FUNCTION__, .cat = CAT_SYS_YIELD, .md = md));
    intercept_capture(ctx(.func = __FUNCTION__, .cat = CAT_EVEC_WAIT, .md = md,
                          .args = {arg_ptr(addr)}));
}

enum lotto_timed_wait_status
_lotto_evec_timed_wait(void *addr, const struct timespec *restrict abstime,
                       metadata_t *md)
{
    intercept_capture(
        ctx(.func = __FUNCTION__, .cat = CAT_SYS_YIELD, .md = md));
    enum lotto_timed_wait_status ret;
    intercept_capture(
        ctx(.func = __FUNCTION__, .cat = CAT_EVEC_TIMED_WAIT, .md = md,
            .args = {arg_ptr(addr), arg_ptr(abstime), arg_ptr(&ret)}));
    return ret;
}

void
_lotto_evec_cancel(void *addr, metadata_t *md)
{
    intercept_capture(
        ctx(.func = __FUNCTION__, .cat = CAT_SYS_YIELD, .md = md));
    intercept_capture(ctx(.func = __FUNCTION__, .cat = CAT_EVEC_CANCEL,
                          .md = md, .args = {arg_ptr(addr)}));
}

void
_lotto_evec_wake(void *addr, uint32_t cnt, metadata_t *md)
{
    intercept_capture(
        ctx(.func = __FUNCTION__, .cat = CAT_SYS_YIELD, .md = md));
    intercept_capture(ctx(.func = __FUNCTION__, .cat = CAT_EVEC_WAKE, .md = md,
                          .args = {arg_ptr(addr), arg(uint32_t, cnt)}));
}

void
_lotto_evec_move(void *src, void *dst, metadata_t *md)
{
    intercept_capture(
        ctx(.func = __FUNCTION__, .cat = CAT_SYS_YIELD, .md = md));
    intercept_capture(ctx(.func = __FUNCTION__, .cat = CAT_EVEC_MOVE, .md = md,
                          .args = {arg_ptr(src), arg_ptr(dst)}));
}

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_COND_WAIT, {
    struct pthread_cond_wait_event *ev = EVENT_PAYLOAD(event);
    _lotto_evec_prepare(ev->cond, md);
    _lotto_mutex_unlock("pthread_cond_wait", ev->mutex, md);
    _lotto_evec_wait(ev->cond, md);
    _lotto_mutex_lock("pthread_cond_wait", ev->mutex, md);
    ev->func = pthread_nop_zero_;
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_COND_TIMEDWAIT, {
    struct pthread_cond_timedwait_event *ev = EVENT_PAYLOAD(event);
    enum lotto_timed_wait_status ret;
    _lotto_evec_prepare(ev->cond, md);
    _lotto_mutex_unlock("pthread_cond_timedwait", ev->mutex, md);
    ret = _lotto_evec_timed_wait(ev->cond, ev->abstime, md);
    _lotto_mutex_lock("pthread_cond_timedwait", ev->mutex, md);
    ev->func = (ret == TIMED_WAIT_TIMEOUT) ? pthread_nop_ETIMEDOUT_ :
                                             pthread_nop_zero_;
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_COND_SIGNAL, {
    struct pthread_cond_timedwait_event *ev = EVENT_PAYLOAD(event);
    _lotto_evec_wake(ev->cond, 1, md);
    ev->func = pthread_nop_zero_;
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_COND_BROADCAST, {
    struct pthread_cond_timedwait_event *ev = EVENT_PAYLOAD(event);
    _lotto_evec_wake(ev->cond, ~((uint32_t)0), md);
    ev->func = pthread_nop_zero_;
    return PS_OK;
})

#if 0
struct timespec64 {
    long long int tv_sec;
    long int tv_nsec;
};

int
___pthread_cond_clockwait64(pthread_cond_t *cond, pthread_mutex_t *mutex,
                            clockid_t clockid, const struct timespec64 *abstime)
{
    struct timespec ts = {.tv_sec  = (time_t)abstime->tv_sec,
                          .tv_nsec = abstime->tv_nsec};
    ASSERT(ts.tv_sec == abstime->tv_sec && "timespec overflow");
    return pthread_cond_timedwait(cond, mutex, &ts);
}
#endif


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
    context_t *c = ctx(.cat = CAT_BEFORE_READ, .md = md);
    intercept_capture(c);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_EVENT, EVENT_MA_WRITE, {
    context_t *c = ctx(.cat = CAT_BEFORE_WRITE, .md = md);
    intercept_capture(c);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_MA_AREAD, {
    context_t *c = ctx(.cat = CAT_BEFORE_AREAD, .md = md);
    intercept_capture(c);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_AFTER, EVENT_MA_AREAD, {
    context_t *c = ctx(.cat = CAT_AFTER_AREAD, .md = md);
    intercept_capture(c);
    return PS_OK;
})
