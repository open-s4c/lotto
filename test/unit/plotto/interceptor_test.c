#include <dlfcn.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <lotto/base/context.h>
#include <lotto/base/plan.h>
#include <lotto/base/record.h>
#include <lotto/brokers/catmgr.h>
#include <lotto/brokers/pubsub.h>
#include <lotto/engine/dispatcher.h>
#include <lotto/runtime/intercept.h>
#include <lotto/runtime/mediator.h>
#include <lotto/states/handlers/deadlock.h>
#include <lotto/states/handlers/mutex.h>
#include <lotto/states/handlers/region_preemption.h>
#include <lotto/sys/ensure.h>
#include <lotto/sys/stdlib.h>

static mediator_t _mediator =
    (mediator_t){.id = 1, .registration_status = MEDIATOR_REGISTRATION_NEED};

/*******************************************************************************
 * pthread_mutex_lock
 ******************************************************************************/
static int
mock_pthread_mutex_lock(pthread_mutex_t *m)
{
    printf("mutex_lock called %p\n", m);
    return 1;
}
static void
test_pthread_mutex_lock()
{
    pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
    printf("call pthread_mutex_lock %p\n", &m);
    int r = pthread_mutex_lock(&m);
    ENSURE(r == 1);
}

/*******************************************************************************
 * pthread_rwlock_rwlock
 ******************************************************************************/
static int
mock_pthread_rwlock_wrlock(pthread_rwlock_t *m)
{
    printf("rwlock_lock called %p\n", m);
    return 1;
}
static void
test_pthread_rwlock_wrlock()
{
    pthread_rwlock_t m = PTHREAD_RWLOCK_INITIALIZER;
    printf("call pthread_rwlock_wrlock %p\n", &m);
    int r = pthread_rwlock_wrlock(&m);
    ENSURE(r == 1);
}

/*******************************************************************************
 * time
 ******************************************************************************/
void
mock_time(void)
{
    printf("time called\n");
}
void
test_time()
{
    time(NULL);
    printf("hello world\n");
}

/*******************************************************************************
 * test cases
 ******************************************************************************/
typedef void *(*foo)(void);

typedef struct {
    const char *func;
    foo test;
    foo mock;
} testcase_t;

#define TESTCASE(N)                                                            \
    {                                                                          \
        .func = #N, .test = (foo)test_##N, .mock = (foo)mock_##N               \
    }

testcase_t tests[] = {
    TESTCASE(pthread_mutex_lock),
    TESTCASE(time),
    TESTCASE(pthread_rwlock_wrlock),
    {NULL},
};

/*******************************************************************************
 * test driver and lotto mock
 ******************************************************************************/
int
main()
{
    testcase_t *nxt = tests;
    for (; nxt->func; nxt++) {
        printf("Testing %s\n", nxt->func);
        nxt->test();
    }
    lotto_intercept_fini();
    return 0;
}

void
lotto_exit(context_t *ctx, reason_t reason)
{
}

void
mediator_fini(mediator_t *m)
{
}

bool
mediator_detach(mediator_t *m)
{
    return true;
}
bool
mediator_attach(mediator_t *m)
{
    return true;
}

bool
mediator_detached(const mediator_t *m)
{
    return false;
}

bool
mediator_in_capture(const mediator_t *m)
{
    return false;
}

uint64_t
engine_get_cur_ns(void)
{
    return 0;
}

uint64_t
prng_next(void)
{
    return 0;
}

bool
mediator_capture(mediator_t *m, context_t *ctx)
{
    printf("intercept %s cat: %s\n", ctx->func, category_str(ctx->cat));
    static int calls = 0;
    switch (calls++) {
        case 0:
            ENSURE(ctx->cat == CAT_TASK_INIT);
            return false;
        case 1:
            ENSURE(ctx->cat == CAT_RSRC_ACQUIRING);
            m->plan = (plan_t){
                .next    = ctx->id,
                .actions = ACTION_CONTINUE,
            };
            return true;
        default:
            ENSURE(ctx->cat == CAT_CALL);
            m->plan = (plan_t){
                .next    = ANY_TASK,
                .actions = ACTION_RETURN | ACTION_YIELD | ACTION_RESUME,
            };
            return true;
    }
    return false;
}

mediator_status_t
mediator_resume(mediator_t *m, context_t *ctx)
{
    static unsigned int counter = 0;
    switch (counter++) {
        case 0:
            ENSURE(ctx->cat == CAT_NONE);
            return MEDIATOR_OK;
            break;
        case 1:
            ENSURE(ctx->cat == CAT_TASK_INIT);
            return MEDIATOR_OK;
            break;
        default:
            break;
    }

    ENSURE(ctx->cat == CAT_CALL && "expecting CAT_CALL");
    printf("return %s\n", ctx->func);

    ENSURE(plan_next(m->plan) == ACTION_YIELD && "wrong plan");
    plan_done(&m->plan);
    return MEDIATOR_OK;
}

void
mediator_return(mediator_t *m, context_t *ctx)
{
    ENSURE(ctx->cat == CAT_CALL && "expecting CAT_CALL");
    ENSURE(plan_next(m->plan) == ACTION_RETURN && "wrong plan");
    plan_done(&m->plan);
}

mediator_t *
mediator_init()
{
    mediator_t *m = sys_malloc(sizeof(mediator_t));
    (*m)          = (mediator_t){.id = 1};
    return m;
}

mediator_t *
mediator_get_data(bool new_task)
{
    return &_mediator;
}

void
engine_resume(const context_t *ctx)
{
}

void
_error_not_found(void)
{
    fprintf(stderr, "called function not found\n");
    abort();
}

/*
static void *
mock_malloc(size_t size)
{
#define BUFSIZE 1024
    static char buffer[BUFSIZE];
    static size_t next;
    ENSURE(next + size < BUFSIZE);
    void *p = buffer + next;
    next += size;
    return p;
}

static struct support {
    const char *func;
    foo fptr;
} support[] = {{"malloc", (foo)mock_malloc}, {NULL}};

void *
real_func(const char *func, const char *lib)
{
    (void)lib;
    testcase_t *nxt = tests;
    for (; nxt->func; nxt++) {
        if (strcmp(func, nxt->func) == 0)
            return nxt->mock;
    }

    struct support *snxt = support;
    for (; snxt->func; snxt++) {
        if (strcmp(func, snxt->func) == 0)
            return snxt->fptr;
    }

    return _error_not_found;
}

void *
real_func_try(const char *func, const char *lib)
{
    return real_func(func, lib);
}
*/
static mutex_config_t _mutex_config;

mutex_config_t *
mutex_config()
{
    return &_mutex_config;
}

region_preemption_config_t *
region_preemption_config()
{
    return NULL;
}

static deadlock_config_t _deadlock_config = {
    .enabled = true,
};

deadlock_config_t *
deadlock_config()
{
    return &_deadlock_config;
}
