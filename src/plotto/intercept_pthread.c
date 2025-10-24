/*
 */
#include <crep.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>

#include <lotto/base/category.h>
#include <lotto/evec.h>
#include <lotto/mutex.h>
#include <lotto/rsrc_deadlock.h>
#include <lotto/runtime/intercept.h>
#include <lotto/states/handlers/deadlock.h>
#include <lotto/states/handlers/mutex.h>
#include <lotto/sys/abort.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/ensure.h>
#include <lotto/sys/logger.h>
#include <lotto/sys/real.h>
#include <lotto/sys/stdlib.h>
#include <lotto/unsafe/disable.h>
#include <lotto/unsafe/rogue.h>
#include <lotto/yield.h>
#include <vsync/atomic/core.h>
#include <vsync/atomic/dispatch.h>
#include <vsync/vtypes.h>

typedef struct {
    void *(*run)(void *);
    void *arg;
    const pthread_attr_t *attr;
} trampoline_t;

static void *
_trampoline_run(void *arg)
{
    trampoline_t *t            = (trampoline_t *)arg;
    void *_arg                 = t->arg;
    void *(*_run)(void *)      = t->run;
    const pthread_attr_t *attr = t->attr;
    sys_free(arg);
    int detachstate;
    if (attr == NULL) {
        detachstate = PTHREAD_CREATE_JOINABLE;
    } else {
        ENSURE(pthread_attr_getdetachstate(attr, &detachstate) == 0);
    }
    (void)intercept_capture(
        ctx(.func = __FUNCTION__, .cat = CAT_TASK_INIT,
            .args = {arg(uint64_t, pthread_self()),
                     arg(bool, detachstate == PTHREAD_CREATE_DETACHED)}));
    void *ret = _run(_arg);
    return ret;
}


void _lotto_enable_unregistered();

int
pthread_create(pthread_t *thread, const pthread_attr_t *attr,
               void *(*run)(void *), void *arg)
{
    REAL_INIT(int, pthread_create, pthread_t *thread,
              const pthread_attr_t *attr, void *(*start_routine)(void *),
              void *arg);

    void *(*run_func)(void *) = _trampoline_run;
    context_t *ctx = ctx(.func = __FUNCTION__, .cat = CAT_TASK_CREATE);
    (void)intercept_before_call(ctx);
    trampoline_t *t = sys_malloc(sizeof(trampoline_t));
    *t              = (trampoline_t){.arg = arg, .run = run, .attr = attr};
    int ret         = REAL(pthread_create, thread, attr, run_func, t);
    ASSERT(ret == 0);
    intercept_after_call(__FUNCTION__);
    return ret;
}

int
pthread_mutex_lock(pthread_mutex_t *mutex)
{
    if (mutex_config()->enabled) {
        _lotto_mutex_acquire_named(__FUNCTION__, mutex);
        return 0;
    }
    int ret = 0;
    if (deadlock_config()->enabled) {
        lotto_rsrc_acquiring(mutex);
    }
    REAL_INIT(int, pthread_mutex_lock, pthread_mutex_t *mutex);
    intercept_before_call(
        ctx(.func = __FUNCTION__, .cat = CAT_CALL, .args = {arg_ptr(mutex)}));

    ret = REAL(pthread_mutex_lock, mutex);
    intercept_after_call(__FUNCTION__);
    return ret;
}

int
pthread_mutex_timedlock(pthread_mutex_t *mutex,
                        const struct timespec *abs_timeout)
{
    if (mutex_config()->enabled) {
        _lotto_mutex_acquire_named(__FUNCTION__, mutex);
        return 0;
    }
    int ret = 0;
    if (deadlock_config()->enabled) {
        lotto_rsrc_acquiring(mutex);
    }
    REAL_INIT(int, pthread_mutex_timedlock, pthread_mutex_t *mutex,
              const struct timespec *abs_timeout);
    intercept_before_call(
        ctx(.func = __FUNCTION__, .cat = CAT_CALL, .args = {arg_ptr(mutex)}));

    ret = REAL(pthread_mutex_timedlock, mutex, abs_timeout);
    intercept_after_call(__FUNCTION__);
    return ret;
}
int
pthread_mutex_trylock(pthread_mutex_t *mutex)
{
    if (mutex_config()->enabled) {
        return _lotto_mutex_tryacquire_named(__FUNCTION__, mutex);
    }
    int ret = 0;
    REAL_INIT(int, pthread_mutex_trylock, pthread_mutex_t *mutex);
    ret = REAL(pthread_mutex_trylock, mutex);
    if (deadlock_config()->enabled) {
        (void)lotto_rsrc_tried_acquiring(mutex, ret == 0);
    }
    return ret;
}

int
pthread_mutex_unlock(pthread_mutex_t *mutex)
{
    if (mutex_config()->enabled) {
        _lotto_mutex_release_named(__FUNCTION__, mutex);
        return 0;
    }
    int ret = 0;
    REAL_INIT(int, pthread_mutex_unlock, pthread_mutex_t *mutex);
    intercept_before_call(
        ctx(.func = __FUNCTION__, .cat = CAT_CALL, .args = {arg_ptr(mutex)}));

    ret = REAL(pthread_mutex_unlock, mutex);
    intercept_after_call(__FUNCTION__);
    if (deadlock_config()->enabled) {
        lotto_rsrc_released(mutex);
    }
    return ret;
}


int
pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
    if (mutex_config()->enabled) {
        lotto_evec_prepare(cond);
        pthread_mutex_unlock(mutex);
        lotto_evec_wait(cond);
        pthread_mutex_lock(mutex);
        return 0;
    }
    int ret = 0;
    REAL_INIT(int, pthread_cond_wait, pthread_cond_t *cond,
              pthread_mutex_t *mutex);
    intercept_before_call(
        ctx(.func = __FUNCTION__, .cat = CAT_CALL, .args = {arg_ptr(mutex)}));

    ret = REAL(pthread_cond_wait, cond, mutex);
    intercept_after_call(__FUNCTION__);
    return ret;
}

int
pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex,
                       const struct timespec *abstime)
{
    if (mutex_config()->enabled) {
        enum lotto_timed_wait_status ret;
        lotto_evec_prepare(cond);
        pthread_mutex_unlock(mutex);
        ret = lotto_evec_timed_wait(cond, abstime);
        pthread_mutex_lock(mutex);
        return ret == TIMED_WAIT_TIMEOUT ? ETIMEDOUT : 0;
    }
    int ret = 0;
    REAL_INIT(int, pthread_cond_timedwait, pthread_cond_t *cond,
              pthread_mutex_t *mutex, const struct timespec *abstime);
    intercept_before_call(
        ctx(.func = __FUNCTION__, .cat = CAT_CALL, .args = {arg_ptr(mutex)}));

    ret = REAL(pthread_cond_timedwait, cond, mutex, abstime);
    intercept_after_call(__FUNCTION__);
    return ret;
}

int
pthread_cond_signal(pthread_cond_t *cond)
{
    if (mutex_config()->enabled) {
        lotto_evec_wake(cond, 1);
        return 0;
    }
    REAL_INIT(int, pthread_cond_signal, pthread_cond_t *cond);
    return REAL(pthread_cond_signal, cond);
}

int
pthread_cond_broadcast(pthread_cond_t *cond)
{
    if (mutex_config()->enabled) {
        lotto_evec_wake(cond, ~((uint32_t)0));
        return 0;
    }
    REAL_INIT(int, pthread_cond_broadcast, pthread_cond_t *cond);
    return REAL(pthread_cond_broadcast, cond);
}

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

int
pthread_key_create(pthread_key_t *key, void (*destructor)(void *))
{
    REAL_INIT(int, pthread_key_create, pthread_key_t *key,
              void (*destructor)(void *));
    int ret = REAL(pthread_key_create, key, NULL);
    if (ret == 0) {
        intercept_capture(ctx(.func = __FUNCTION__, .cat = CAT_KEY_CREATE,
                              .args = {arg_ptr(key), arg_ptr(destructor)}));
    }
    ASSERT(ret == 0);
    return ret;
}

int
pthread_key_delete(pthread_key_t key)
{
    intercept_capture(ctx(.func = __FUNCTION__, .cat = CAT_KEY_DELETE,
                          .args = {arg_ptr(&key)}));
    REAL_INIT(int, pthread_key_delete, pthread_key_t key);
    return REAL(pthread_key_delete, key);
}

int
pthread_setspecific(pthread_key_t key, const void *value)
{
    intercept_capture(ctx(.func = __FUNCTION__, .cat = CAT_SET_SPECIFIC,
                          .args = {arg_ptr(&key), arg_ptr(value)}));
    REAL_INIT(int, pthread_setspecific, pthread_key_t key, const void *value);
    return REAL(pthread_setspecific, key, value);
}

int
pthread_join(pthread_t thread, void **value_ptr)
{
    int ret = EINTR;
    intercept_capture(ctx(.func = __FUNCTION__, .cat = CAT_JOIN,
                          .args = {arg(uint64_t, thread), arg_ptr(value_ptr),
                                   arg_ptr(&ret)}));
    ASSERT(ret != EINTR);
    return ret;
}

void
pthread_exit(void *value_ptr)
{
    intercept_capture(ctx(.func = __FUNCTION__, .cat = CAT_EXIT,
                          .args = {arg_ptr(value_ptr)}));
    REAL_INIT(void, pthread_exit, void *value_ptr);
    REAL(pthread_exit, value_ptr);
    ASSERT(0 && "should not reach here");
    while (1)
        ;
}

int
pthread_detach(pthread_t thread)
{
    int ret = EINTR;
    intercept_capture(ctx(.func = __FUNCTION__, .cat = CAT_DETACH,
                          .args = {arg(uint64_t, thread), arg_ptr(&ret)}));
    ASSERT(ret != EINTR);
    return ret;
}
