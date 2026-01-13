/*
 */
#include <lotto/base/context.h>
#include <lotto/base/task_id.h>
#include <lotto/engine/engine.h>
#include <lotto/engine/prng.h>
#include <lotto/runtime/mediator.h>
#include <lotto/runtime/runtime.h>
#include <lotto/runtime/switcher.h>
#include <lotto/states/sequencer.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/ensure.h>
#include <lotto/sys/pthread.h>
#include <lotto/sys/stdlib.h>
#include <lotto/util/once.h>
#include <vsync/atomic.h>
#include <vsync/atomic/dispatch.h>

#define CAPTURE_ALLOWED(m, ctx)                                                \
    ((m)->registration_status == MEDIATOR_REGISTRATION_DONE ||                 \
     ((m)->registration_status == MEDIATOR_REGISTRATION_EXEC &&                \
      (ctx)->cat == CAT_TASK_INIT))

task_id next_task_id(void);
bool _enter_capture(mediator_t *m);
bool _leave_capture(mediator_t *m);
bool _nested_capture(mediator_t *m);
static inline bool _guard_capture(mediator_t *m, context_t *ctx);

#define MEDIATOR_DESTRUCTOR_CAP 1024

size_t ndestructors;
static struct mediator_destructor {
    pthread_key_t key;
    void (*destructor)(void *);
} destructors[MEDIATOR_DESTRUCTOR_CAP];

/* thread-specific data key(visible to all threads of same process) */
static pthread_key_t key;
static vatomic32_t _registration_enabled;

static LOTTO_CONSTRUCTOR void
_lotto_mediator_init(void)
{
    vatomic_init(&_registration_enabled, true);
}

static bool
_mediator_registration_enabled()
{
    return vatomic_xchg(&_registration_enabled, false);
}

static void
_mediator_dtor(void *arg)
{
    mediator_t *m = (mediator_t *)arg;
    ASSERT(m->guards.capture == 0);
    ENSURE(sys_pthread_setspecific(key, m) == 0);
    for (size_t i; m->nvalues > 0; m->values[i] = m->values[--m->nvalues]) {
        i                        = prng_range(0, m->nvalues);
        struct mediator_value *v = &m->values[i];
        if (v->value == NULL) {
            continue;
        }
        for (size_t j = 0; j < ndestructors; j++) {
            struct mediator_destructor *d = &destructors[j];
            if (d->destructor == NULL || d->key != v->key) {
                continue;
            }
            d->destructor(v->value);
            break;
        }
    }
    sys_pthread_setspecific(key, NULL);

    if (m->registration_status == MEDIATOR_REGISTRATION_DONE) {
        context_t *ctx =
            ctx(.id = m->id, .func = __FUNCTION__, .cat = CAT_TASK_FINI);
        if (!mediator_capture(m, ctx)) {
            switch (mediator_resume(m, ctx)) {
                case MEDIATOR_OK:
                    break;
                case MEDIATOR_ABORT:
                    lotto_exit(ctx, REASON_ABORT);
                    break;
                case MEDIATOR_SHUTDOWN:
                    lotto_exit(ctx, REASON_SHUTDOWN);
                    break;
                default:
                    logger_fatalf("unexpected mediator resume output\n");
                    break;
            };
        }
    }

    sys_free(arg);
}

/*
 * The key must to be initialized only ONCE before usage.
 *
 * Warning: if this function is inline and called in
 * multiple locations, once() will not be exclusive!
 */
static __attribute__((noinline)) void
once_init_key(void)
{
    once({
        ENSURE(sys_pthread_key_create(&key, _mediator_dtor) == 0 &&
               "pthread_key_create failed");
    });
}

inline __attribute__((always_inline)) mediator_t *
mediator_get_data(bool new_task)
{
    once_init_key();
    mediator_t *m = (mediator_t *)pthread_getspecific(key);
    if (m == NULL) {
        m = mediator_init();
    }
    if (m->registration_status != MEDIATOR_REGISTRATION_NONE) {
        return m;
    }
    bool register_main = _mediator_registration_enabled();
    ASSERT(!(register_main && new_task) && "task creation was not intercepted");
    if (!(register_main || new_task)) {
        return m;
    }
    m->registration_status = MEDIATOR_REGISTRATION_NEED;
    /* make resume possible */
    _enter_capture(m);
    /* attach the task */
    mediator_attach(m);
    ASSERT(!mediator_detached(m));
    return m;
}

mediator_t *
mediator_init()
{
    /* allocate mediator thread-specific storage */
    mediator_t *m = sys_malloc(sizeof(mediator_t));
    ASSERT((m != NULL) && "sys_malloc failed");
    *m = (mediator_t){.id                  = next_task_id(),
                      .plan.actions        = ACTION_RESUME,
                      .optimization        = MEDIATOR_OPTIMIZATION_NONE,
                      .finito              = false,
                      .registration_status = MEDIATOR_REGISTRATION_NONE,
                      .guards.detach       = 1};
    once_init_key();
    ENSURE(sys_pthread_setspecific(key, m) == 0 &&
           "pthread_setspecific failed");

    return m;
}

void
mediator_disable_registration()
{
    vatomic_write(&_registration_enabled, false);
}

static void
_plan_optimize(plan_t *plan, const context_t *ctx,
               mediator_optimization_t optimization)
{
    if (plan->next != ctx->id ||
        (plan->actions != (ACTION_WAKE | ACTION_YIELD | ACTION_RESUME))) {
        return;
    }
    switch (optimization) {
        case MEDIATOR_OPTIMIZATION_NONE:
            return;
        case MEDIATOR_OPTIMIZATION_SAME_TASK_EXCEPT_ANY:
            if (plan->replay_type == REPLAY_ANY_TASK) {
                return;
            }
            // fallthru
        case MEDIATOR_OPTIMIZATION_SAME_TASK:
            plan->actions = ACTION_RESUME | ACTION_CONTINUE;
            break;
        default:
            ASSERT(0 && "unknown mediator optimization");
            break;
    }
}

bool
mediator_is_any_task(mediator_t *m)
{
    return m->plan.replay_type == REPLAY_ANY_TASK;
}

static inline bool
_guard_capture(mediator_t *m, context_t *ctx)
{
    return _enter_capture(m) && !mediator_detached(m) &&
           CAPTURE_ALLOWED(m, ctx);
}

bool
mediator_capture(mediator_t *m, context_t *ctx)
{
    ASSERT(!m->finito);
    ctx->id = m->id;
    if (!_guard_capture(m, ctx)) {
        switch (ctx->cat) {
            case CAT_CALL:
            case CAT_TASK_CREATE:
                mediator_detach(m);
                return true;
            default:
                break;
        }
        return false;
    }

    switch (ctx->cat) {
        case CAT_KEY_CREATE:
            ASSERT(ndestructors < MEDIATOR_DESTRUCTOR_CAP &&
                   "increase MEDIATOR_DESTRUCTOR_CAP");
            destructors[ndestructors++] = (struct mediator_destructor){
                .key        = *(pthread_key_t *)ctx->args[0].value.ptr,
                .destructor = (void (*)(void *))ctx->args[1].value.ptr};
            _leave_capture(m);
            return true;

        case CAT_KEY_DELETE: {
            pthread_key_t key = *(pthread_key_t *)ctx->args[0].value.ptr;
            for (size_t i = 0; i < ndestructors; i++) {
                if (destructors[i].key != key) {
                    continue;
                }
                destructors[i] = destructors[--ndestructors];
                break;
            }
            _leave_capture(m);
            return true;
        }

        case CAT_SET_SPECIFIC: {
            struct mediator_value value = (struct mediator_value){
                .key   = *(pthread_key_t *)ctx->args[0].value.ptr,
                .value = (void *)ctx->args[1].value.ptr};
            for (size_t i = 0; i < m->nvalues; i++) {
                struct mediator_value *v = m->values + i;
                if (v->key != value.key) {
                    continue;
                }
                *v = value;
                _leave_capture(m);
                return true;
            }
            size_t i = m->nvalues++;
            ASSERT(i < MEDIATOR_VALUE_CAP && "increase MEDIATOR_VALUE_CAP");
            m->values[i] = value;
            _leave_capture(m);
            return true;
        }

        default:
            break;
    }

    m->plan = engine_capture(ctx);
    _plan_optimize(&m->plan, ctx, m->optimization);

    do
        switch (plan_next(m->plan)) {
            case ACTION_WAKE:
                switcher_wake(m->plan.next,
                              m->plan.with_slack ?
                                  sequencer_config()->slack * NOW_MILLISECOND :
                                  0);
                break;

            case ACTION_CALL:
                mediator_detach(m);
                // fallthru
            case ACTION_BLOCK:
                if (plan_done(&m->plan))
                    ASSERT(0 && "expected plan not to be done");
                return true;

            case ACTION_CONTINUE:
            case ACTION_YIELD:
            case ACTION_RESUME:
            case ACTION_SHUTDOWN:
                return false;

            default:
                logger_fatalf("unexpected action: %s\n",
                           action_str(plan_next(m->plan)));
        }
    while (!plan_done(&m->plan));

    // only when the task finishes
    ASSERT(ctx->cat == CAT_TASK_FINI);

    return true;
}

static inline bool
_guard_resume(mediator_t *m)
{
    return !_nested_capture(m) && !mediator_detached(m);
}

mediator_status_t
mediator_resume(mediator_t *m, context_t *ctx)
{
    if (!_guard_resume(m)) {
        _leave_capture(m);
        return MEDIATOR_OK;
    }
    ASSERT(!m->finito);
    ASSERT(plan_has(m->plan, ACTION_RESUME) ||
           plan_has(m->plan, ACTION_CONTINUE) ||
           plan_has(m->plan, ACTION_SHUTDOWN));

    ctx->id              = m->id;
    mediator_status_t st = MEDIATOR_OK;

    do
        switch (plan_next(m->plan)) {
            case ACTION_YIELD:
                switcher_yield(ctx->id, m->plan.any_task_filter);
                break;

            case ACTION_RESUME:
                engine_resume(ctx);
                break;

            case ACTION_SHUTDOWN:
                ASSERT(IS_REASON_SHUTDOWN(m->plan.reason) ||
                       IS_REASON_ABORT(m->plan.reason));
                st = IS_REASON_SHUTDOWN(m->plan.reason) ? MEDIATOR_SHUTDOWN :
                                                          MEDIATOR_ABORT;
                break;

            case ACTION_CONTINUE:
                break;

            default:
                logger_fatalf("unexpected action: %s\n",
                           action_str(plan_next(m->plan)));
        }
    while (!plan_done(&m->plan));
    _leave_capture(m);

    ASSERT(plan_next(m->plan) == ACTION_NONE);
    return st;
}

static inline bool
_should_resume(const mediator_t *m)
{
    action_t a = plan_next(m->plan);
    return a == ACTION_RESUME || a == ACTION_YIELD;
}

static inline bool
_guard_return(mediator_t *m)
{
    return mediator_attach(m) &&
           m->registration_status == MEDIATOR_REGISTRATION_DONE &&
           !_nested_capture(m);
}

void
mediator_return(mediator_t *m, context_t *ctx)
{
    if (!_guard_return(m)) {
        return;
    }
    ASSERT(!m->finito);
    ctx->id = m->id;

    if (ctx->cat == CAT_TASK_CREATE) {
        ASSERT(_should_resume(m));
        return;
    }
    ASSERT(plan_next(m->plan) == ACTION_RETURN);
    engine_return(ctx);
    plan_done(&m->plan);
}

void
mediator_fini(mediator_t *m)
{
    if (m == NULL)
        return;
    ASSERT(!m->finito);
    m->finito = true;
}

bool
_enter_capture(mediator_t *m)
{
    ASSERT(m->guards.capture >= 0);
    return 0 == m->guards.capture++;
}

bool
_leave_capture(mediator_t *m)
{
    ASSERT(m->guards.capture > 0);
    return 0 == --m->guards.capture;
}

bool
_nested_capture(mediator_t *m)
{
    return m->guards.capture > 1;
}

bool
mediator_in_capture(const mediator_t *m)
{
    return m->guards.capture > 0;
}

bool
mediator_detach(mediator_t *m)
{
    ASSERT(m->guards.detach >= 0);
    return 0 == m->guards.detach++;
}

bool
mediator_attach(mediator_t *m)
{
    ASSERT(mediator_detached(m));
    return 0 == --m->guards.detach;
}

bool
mediator_detached(const mediator_t *m)
{
    return m->guards.detach > 0;
}
