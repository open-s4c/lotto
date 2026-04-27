#include <dice/chains/intercept.h>
#include <dice/events/dice.h>
#include <dice/module.h>
#include <dice/self.h>
#include <lotto/base/task_id.h>
#include <lotto/engine/engine.h>
#include <lotto/engine/pubsub.h>
#include <lotto/engine/state.h>
#include <lotto/runtime/capture_point.h>
#include <lotto/runtime/events.h>
#include <lotto/runtime/mediator.h>
#include <lotto/runtime/runtime.h>
#include <lotto/runtime/switcher.h>
#include <lotto/sys/assert.h>
#include <vsync/atomic.h>
#include <vsync/atomic/dispatch.h>

static mediator_t mediator_key_;

LOTTO_ADVERTISE_TYPE(EVENT_RUNTIME__NOP)
static void
ensure_ps_intialized_(void)
{
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_RUNTIME__NOP, 0, 0);
}

mediator_t *
mediator_get(struct metadata *md, bool bootstrap)
{
    if (md == NULL) {
        ensure_ps_intialized_();
        md = self_md();
    }
    ASSERT(md != NULL);

    mediator_t *m = self_tls_get(md, (uintptr_t)&mediator_key_);
    if (m == NULL) {
        if (!bootstrap)
            return NULL;
        m = SELF_TLS(md, &mediator_key_);
    }
    if (m->id == NO_TASK) {
        *m = (mediator_t){
            .id           = self_id(md),
            .plan.actions = ACTION_RESUME,
            .optimization = MEDIATOR_OPTIMIZATION_NONE,
            .finito       = false,
        };
        if (bootstrap) {
            capture_point cp = {
                .func     = __FUNCTION__,
                .chain_id = CHAIN_INGRESS_EVENT,
                .type_id  = EVENT_DICE_NOP,
            };
            mediator_status_t status = mediator_resume(m, &cp);
            ASSERT(status == MEDIATOR_OK && "mediator bootstrap failed");
        }
    }
    return m;
}

static void
_plan_optimize(struct plan *plan, const capture_point *cp,
               mediator_optimization_t optimization)
{
    if (plan->next != cp->id ||
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


bool
mediator_capture(mediator_t *m, capture_point *cp)
{
    ASSERT(!m->finito);
    cp->id = m->id;

    struct metadata *md = self_md();
    if (self_retired(md))
        return false;

    m->plan = engine_capture(cp);
    _plan_optimize(&m->plan, cp, m->optimization);

    do {
        switch (plan_next(m->plan)) {
            case ACTION_WAKE:
                switcher_wake(m->plan.next,
                              m->plan.with_slack ?
                                  sequencer_config()->slack * NOW_MILLISECOND :
                                  0);
                break;

            case ACTION_BLOCK:
                if (plan_done(&m->plan))
                    logger_fatalf("expected plan not to be done");
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
    } while (!plan_done(&m->plan));

    // only when the task finishes
    ASSERT(cp->type_id == EVENT_TASK_FINI);

    return true;
}


mediator_status_t
mediator_resume(mediator_t *m, capture_point *cp)
{
    ASSERT(!m->finito);
    ASSERT(plan_has(m->plan, ACTION_RESUME) ||
           plan_has(m->plan, ACTION_CONTINUE) ||
           plan_has(m->plan, ACTION_SHUTDOWN));

    cp->id               = m->id;
    mediator_status_t st = MEDIATOR_OK;

    do {
        switch (plan_next(m->plan)) {
            case ACTION_YIELD:
                switcher_yield(cp->id, m->plan.any_task_filter);
                break;

            case ACTION_RESUME:
                engine_resume(cp);
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
    } while (!plan_done(&m->plan));

    ASSERT(plan_next(m->plan) == ACTION_NONE);
    return st;
}

reason_t
mediator_reason(const mediator_t *m)
{
    ASSERT(m != NULL);
    return m->plan.reason;
}

static inline bool
_should_resume(const mediator_t *m)
{
    enum action a = plan_next(m->plan);
    return a == ACTION_RESUME || a == ACTION_YIELD;
}

void
mediator_return(mediator_t *m, capture_point *cp)
{
    ASSERT(!m->finito);
    cp->id = m->id;

    if (cp->type_id == EVENT_TASK_CREATE) {
        ASSERT(_should_resume(m));
        return;
    }
    ASSERT(plan_next(m->plan) == ACTION_RETURN);
    engine_return(cp);
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
mediator_in_capture(const mediator_t *m)
{
    if (m == NULL)
        return false;

    // TODO: mediator should point to md already
    struct metadata *md = self_md();
    return self_guard_get(md) == SELF_GUARD_SERVING;
}
