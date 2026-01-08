#include <stdbool.h>

#include <dice/self.h>
#include <lotto/base/category.h>
#include <lotto/base/context.h>
#include <lotto/engine/engine.h>
#include <lotto/runtime/mediator.h>
#include <lotto/runtime/runtime.h>
#include <lotto/runtime/switcher.h>
#include <lotto/states/sequencer.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger.h>
#include <lotto/sys/now.h>

static mediator_t mediator_key_;

mediator_t *
mediator_tls(metadata_t *md)
{
    if (md == NULL) {
        log_fatal("mediator_tls: missing metadata");
    }

    mediator_t *m = SELF_TLS(md, &mediator_key_);
    if (m == NULL) {
        log_fatal("mediator_tls: allocation failure");
    }

    if (m->registration_status == MEDIATOR_REGISTRATION_NONE) {
        m->id                  = (task_id)self_id(md);
        m->registration_status = MEDIATOR_REGISTRATION_DONE;

        context_t *bootstrap = ctx();
        bootstrap->func      = "mediator_bootstrap";
        bootstrap->cat       = CAT_NONE;
        bootstrap->id        = m->id;
        bootstrap->md        = md;
        engine_resume(bootstrap);
    }

    return m;
}

static inline void
mediator_plan_reset(mediator_t *m)
{
    m->plan.actions         = ACTION_NONE;
    m->plan.any_task_filter = NULL;
    m->plan.next            = NO_TASK;
    m->plan.with_slack      = false;
}

bool
mediator_detach(mediator_t *m)
{
    return m->detach_depth++ == 0;
}

bool
mediator_attach(mediator_t *m)
{
    ASSERT(m->detach_depth > 0);
    return --m->detach_depth == 0;
}

bool
mediator_detached(const mediator_t *m)
{
    return m->detach_depth > 0;
}

bool
mediator_in_capture(const mediator_t *m)
{
    return m->capture_depth > 0;
}

static void
mediator_process_shutdown(mediator_t *m, reason_t reason)
{
    m->finito             = true;
    m->has_pending_reason = true;
    m->pending_reason     = reason;
}

bool
mediator_capture(mediator_t *m, context_t *ctx)
{
    ASSERT(m != NULL);
    ASSERT(ctx != NULL);

    ctx->id = m->id;
    m->capture_depth++;

    if (mediator_detached(m)) {
        m->capture_depth--;
        return false;
    }

    m->plan = engine_capture(ctx);

    while (true) {
        action_t action = plan_next(m->plan);
        if (action == ACTION_NONE) {
            break;
        }
        (void)plan_done(&m->plan);
        switch (action) {
            case ACTION_WAKE: {
                nanosec_t slack =
                    m->plan.with_slack ?
                        sequencer_config()->slack * NOW_MILLISECOND :
                        0;
                switcher_wake(m->plan.next, slack);
                break;
            }
            case ACTION_CALL:
                mediator_detach(m);
                m->capture_depth--;
                return true;
            case ACTION_BLOCK:
                m->capture_depth--;
                return true;
            case ACTION_YIELD:
                switcher_yield(m->id, m->plan.any_task_filter);
                break;
            case ACTION_RESUME:
                engine_resume(ctx);
                break;
            case ACTION_SHUTDOWN:
                mediator_process_shutdown(m, m->plan.reason);
                break;
            case ACTION_CONTINUE:
                break;
            default:
                break;
        }
    }

    m->capture_depth--;
    return false;
}

mediator_status_t
mediator_resume(mediator_t *m, context_t *ctx)
{
    ASSERT(m != NULL);
    ASSERT(ctx != NULL);

    ctx->id = m->id;

    mediator_status_t status = MEDIATOR_OK;

    while (true) {
        action_t action = plan_next(m->plan);
        if (action == ACTION_NONE) {
            break;
        }
        (void)plan_done(&m->plan);
        switch (action) {
            case ACTION_YIELD:
                switcher_yield(m->id, m->plan.any_task_filter);
                break;
            case ACTION_RESUME:
                engine_resume(ctx);
                break;
            case ACTION_SHUTDOWN:
                mediator_process_shutdown(m, m->plan.reason);
                status = IS_REASON_SHUTDOWN(m->plan.reason) ?
                             MEDIATOR_SHUTDOWN :
                             MEDIATOR_ABORT;
                break;
            case ACTION_CONTINUE:
                break;
            default:
                break;
        }
    }

    mediator_plan_reset(m);

    if (m->has_pending_reason) {
        reason_t reason       = m->pending_reason;
        m->has_pending_reason = false;
        lotto_exit(ctx, reason);
    }

    return status;
}

void
mediator_return(mediator_t *m, context_t *ctx)
{
    ASSERT(m != NULL);
    ASSERT(ctx != NULL);
    ctx->id = m->id;
    if (!mediator_attach(m)) {
        return;
    }
    if (plan_next(m->plan) == ACTION_RETURN) {
        (void)plan_done(&m->plan);
        engine_return(ctx);
    }
}

void
mediator_fini(mediator_t *m)
{
    if (m == NULL)
        return;
    m->finito = true;
}

void
mediator_disable_registration(void)
{
    /* No-op in the trimmed runtime. */
}
