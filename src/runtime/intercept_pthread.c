#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#include <lotto/base/category.h>
#include <lotto/base/context.h>
#include <lotto/evec.h>
#include <lotto/rsrc_deadlock.h>
#include <lotto/runtime/intercept.h>
#include <lotto/sys/real.h>

static inline void
capture_simple(const char *func, category_t cat, arg_t a0, arg_t a1)
{
    if (intercept_capture != NULL) {
        context_t *c = ctx();
        c->func      = func;
        c->cat       = cat;
        c->args[0]   = a0;
        c->args[1]   = a1;
        intercept_capture(c);
    }
}

int
pthread_create(pthread_t *thread, const pthread_attr_t *attr,
               void *(*start)(void *), void *arg)
{
    context_t *c = ctx();
    c->func      = __FUNCTION__;
    c->cat       = CAT_TASK_CREATE;
    c->args[0] = arg_ptr(thread);
    c->args[1] = arg_ptr(attr);
    c->args[2] = arg_ptr(start);
    c->args[3] = arg_ptr(arg);
    if (intercept_before_call != NULL) {
        intercept_before_call(c);
    }

    REAL_PTHREAD_INIT(int, pthread_create, pthread_t *, const pthread_attr_t *,
                      void *(*)(void *), void *);
    int ret = REAL(pthread_create, thread, attr, start, arg);

    if (intercept_after_call != NULL) {
        intercept_after_call(__FUNCTION__);
    }
    return ret;
}

int
pthread_join(pthread_t thread, void **value_ptr)
{
    capture_simple(__FUNCTION__, CAT_JOIN, arg(uint64_t, (uint64_t)thread),
                   arg_ptr(value_ptr));

    REAL_PTHREAD_INIT(int, pthread_join, pthread_t, void **);
    return REAL(pthread_join, thread, value_ptr);
}

void
pthread_exit(void *value_ptr)
{
    capture_simple(__FUNCTION__, CAT_EXIT, arg_ptr(value_ptr), arg_ptr(NULL));
    REAL_PTHREAD_INIT(void, pthread_exit, void *);
    REAL(pthread_exit, value_ptr);
    __builtin_unreachable();
}

int
pthread_detach(pthread_t thread)
{
    capture_simple(__FUNCTION__, CAT_DETACH, arg(uint64_t, (uint64_t)thread),
                   arg_ptr(NULL));
    REAL_PTHREAD_INIT(int, pthread_detach, pthread_t);
    return REAL(pthread_detach, thread);
}

static inline context_t *
mutex_ctx(const char *func, pthread_mutex_t *mutex)
{
    context_t *c = ctx();
    c->func      = func;
    c->cat       = CAT_CALL;
    c->args[0]   = arg_ptr(mutex);
    return c;
}

int
pthread_mutex_lock(pthread_mutex_t *mutex)
{
    lotto_rsrc_acquiring(mutex);
    if (intercept_before_call != NULL) {
        intercept_before_call(mutex_ctx(__FUNCTION__, mutex));
    }

    REAL_PTHREAD_INIT(int, pthread_mutex_lock, pthread_mutex_t *);
    int ret = REAL(pthread_mutex_lock, mutex);

    if (intercept_after_call != NULL) {
        intercept_after_call(__FUNCTION__);
    }
    return ret;
}

int
pthread_mutex_timedlock(pthread_mutex_t *mutex,
                        const struct timespec *abstime)
{
    lotto_rsrc_acquiring(mutex);
    if (intercept_before_call != NULL) {
        intercept_before_call(mutex_ctx(__FUNCTION__, mutex));
    }

    REAL_PTHREAD_INIT(int, pthread_mutex_timedlock, pthread_mutex_t *,
                      const struct timespec *);
    int ret = REAL(pthread_mutex_timedlock, mutex, abstime);

    if (intercept_after_call != NULL) {
        intercept_after_call(__FUNCTION__);
    }
    if (ret != 0) {
        lotto_rsrc_released(mutex);
    }
    return ret;
}

int
pthread_mutex_trylock(pthread_mutex_t *mutex)
{
    REAL_PTHREAD_INIT(int, pthread_mutex_trylock, pthread_mutex_t *);
    int ret = REAL(pthread_mutex_trylock, mutex);
    (void)lotto_rsrc_tried_acquiring(mutex, ret == 0);
    return ret;
}

int
pthread_mutex_unlock(pthread_mutex_t *mutex)
{
    if (intercept_before_call != NULL) {
        intercept_before_call(mutex_ctx(__FUNCTION__, mutex));
    }

    REAL_PTHREAD_INIT(int, pthread_mutex_unlock, pthread_mutex_t *);
    int ret = REAL(pthread_mutex_unlock, mutex);

    if (ret == 0) {
        lotto_rsrc_released(mutex);
    }
    if (intercept_after_call != NULL) {
        intercept_after_call(__FUNCTION__);
    }
    return ret;
}

static inline context_t *
cond_ctx(const char *func, pthread_cond_t *cond, pthread_mutex_t *mutex)
{
    context_t *c = ctx();
    c->func      = func;
    c->cat       = CAT_CALL;
    c->args[0]   = arg_ptr(cond);
    c->args[1]   = arg_ptr(mutex);
    return c;
}

int
pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
    if (intercept_before_call != NULL) {
        intercept_before_call(cond_ctx(__FUNCTION__, cond, mutex));
    }

    REAL_PTHREAD_INIT(int, pthread_cond_wait, pthread_cond_t *,
                      pthread_mutex_t *);
    int ret = REAL(pthread_cond_wait, cond, mutex);

    if (intercept_after_call != NULL) {
        intercept_after_call(__FUNCTION__);
    }
    return ret;
}

int
pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex,
                       const struct timespec *abstime)
{
    context_t *c = cond_ctx(__FUNCTION__, cond, mutex);
    c->args[2]   = arg_ptr(abstime);
    if (intercept_before_call != NULL) {
        intercept_before_call(c);
    }

    REAL_PTHREAD_INIT(int, pthread_cond_timedwait, pthread_cond_t *,
                      pthread_mutex_t *, const struct timespec *);
    int ret = REAL(pthread_cond_timedwait, cond, mutex, abstime);

    if (intercept_after_call != NULL) {
        intercept_after_call(__FUNCTION__);
    }
    return ret;
}

int
pthread_cond_signal(pthread_cond_t *cond)
{
    capture_simple(__FUNCTION__, CAT_CALL, arg_ptr(cond), arg_ptr(NULL));
    REAL_PTHREAD_INIT(int, pthread_cond_signal, pthread_cond_t *);
    return REAL(pthread_cond_signal, cond);
}

int
pthread_cond_broadcast(pthread_cond_t *cond)
{
    capture_simple(__FUNCTION__, CAT_CALL, arg_ptr(cond), arg_ptr(NULL));
    REAL_PTHREAD_INIT(int, pthread_cond_broadcast, pthread_cond_t *);
    return REAL(pthread_cond_broadcast, cond);
}

int
pthread_key_create(pthread_key_t *key, void (*destructor)(void *))
{
    capture_simple(__FUNCTION__, CAT_KEY_CREATE, arg_ptr(key),
                   arg_ptr(destructor));
    REAL_PTHREAD_INIT(int, pthread_key_create, pthread_key_t *,
                      void (*)(void *));
    return REAL(pthread_key_create, key, destructor);
}

int
pthread_key_delete(pthread_key_t key)
{
    capture_simple(__FUNCTION__, CAT_KEY_DELETE, arg_ptr(&key), arg_ptr(NULL));
    REAL_PTHREAD_INIT(int, pthread_key_delete, pthread_key_t);
    return REAL(pthread_key_delete, key);
}

int
pthread_setspecific(pthread_key_t key, const void *value)
{
    capture_simple(__FUNCTION__, CAT_SET_SPECIFIC,
                   arg_ptr(&key), arg_ptr(value));
    REAL_PTHREAD_INIT(int, pthread_setspecific, pthread_key_t, const void *);
    return REAL(pthread_setspecific, key, value);
}
