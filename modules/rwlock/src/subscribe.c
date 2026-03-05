#include <time.h>
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
#include <lotto/evec.h>
#include <lotto/mutex.h>
#include <lotto/rsrc_deadlock.h>
#include <lotto/runtime/intercept.h>
#include <lotto/sys/logger.h>

static int retval = 0;
static int const_func(pthread_rwlock_t *);
static int const_func_timed(pthread_rwlock_t *, const struct timespec *);
#define CONST(val) ((retval = (val)), const_func)
#define CONST_TIMED(val) ((retval = (val)), const_func_timed)

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_RWLOCK_RDLOCK, {
    struct pthread_rwlock_rdlock_event *ev = EVENT_PAYLOAD(event);
    intercept_capture(ctx_pc(
        .pc = (uintptr_t)ev->pc,
        .cat = CAT_RWLOCK_RDLOCK,
        .func = "pthread_rwlock_rdlock",
        .args = {arg_ptr(ev->lock)},
    ));
    ev->func = CONST(0);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_RWLOCK_WRLOCK, {
    struct pthread_rwlock_wrlock_event *ev = EVENT_PAYLOAD(event);
    intercept_capture(ctx_pc(
        .pc = (uintptr_t)ev->pc,
        .cat = CAT_RWLOCK_WRLOCK,
        .func = "pthread_rwlock_wrlock",
        .args = {arg_ptr(ev->lock)},
    ));
    ev->func = CONST(0);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_RWLOCK_UNLOCK, {
    struct pthread_rwlock_unlock_event *ev = EVENT_PAYLOAD(event);
    intercept_capture(ctx_pc(
        .pc = (uintptr_t)ev->pc,
        .cat = CAT_RWLOCK_UNLOCK,
        .func = "pthread_rwlock_unlock",
        .args = {arg_ptr(ev->lock)},
    ));
    ev->func = CONST(0);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_RWLOCK_TRYRDLOCK, {
    struct pthread_rwlock_tryrdlock_event *ev = EVENT_PAYLOAD(event);
    context_t *ctx = ctx_pc(
        .pc = (uintptr_t)ev->pc,
        .cat = CAT_RWLOCK_TRYRDLOCK,
        .func = "pthread_rwlock_tryrdlock",
        .args = {arg_ptr(ev->lock)},
    );
    intercept_capture(ctx);
    ev->func = CONST((int)ctx->args[1].value.u32);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_RWLOCK_TRYWRLOCK, {
    struct pthread_rwlock_trywrlock_event *ev = EVENT_PAYLOAD(event);
    context_t *ctx = ctx_pc(
        .pc = (uintptr_t)ev->pc,
        .cat = CAT_RWLOCK_TRYWRLOCK,
        .func = "pthread_rwlock_trywrlock",
        .args = {arg_ptr(ev->lock)},
    );
    intercept_capture(ctx);
    ev->func = CONST((int)ctx->args[1].value.u32);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_RWLOCK_TIMEDRDLOCK, {
    struct pthread_rwlock_timedrdlock_event *ev = EVENT_PAYLOAD(event);
    intercept_capture(ctx_pc(
        .pc = (uintptr_t)ev->pc,
        .cat = CAT_RWLOCK_RDLOCK,
        .func = "pthread_rwlock_timedrdlock",
        .args = {arg_ptr(ev->lock), arg_ptr(ev->abstime)},
    ));
    ev->func = CONST_TIMED(0);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_RWLOCK_TIMEDWRLOCK, {
    struct pthread_rwlock_timedwrlock_event *ev = EVENT_PAYLOAD(event);
    intercept_capture(ctx_pc(
        .pc = (uintptr_t)ev->pc,
        .cat = CAT_RWLOCK_WRLOCK,
        .func = "pthread_rwlock_timedwrlock",
        .args = {arg_ptr(ev->lock), arg_ptr(ev->abstime)},
    ));
    ev->func = CONST_TIMED(0);
    return PS_OK;
})

static int
const_func(pthread_rwlock_t *lock)
{
    return retval;
}
static int
const_func_timed(pthread_rwlock_t *lock, const struct timespec *abstime)
{
    return retval;
}
