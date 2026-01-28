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
