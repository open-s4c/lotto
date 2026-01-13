/*
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // _exit

#include <vsync/atomic.h>
#include <vsync/atomic/dispatch.h>
#include <vsync/spinlock/caslock.h>

#define LOGGER_PREFIX LOGGER_CUR_FILE
#define LOGGER_BLOCK  LOGGER_CUR_BLOCK
#include <lotto/base/reason.h>
#include <lotto/base/trace.h>
#include <lotto/brokers/catmgr.h>
#include <lotto/engine/sequencer.h>
#include <lotto/sys/logger_block.h>
#include <lotto/sys/now.h>
#include <lotto/util/contract.h>
#include <lotto/util/once.h>

#define log(ctx, fmt, ...)                                                     \
    logger_debugf("[t:%lu, " CONTRACT(" clk:%lu,") "pc:0x%lx] " fmt "\n",         \
               ctx->id, CONTRACT(_ghost.clk, ) ctx->pc & 0xfff, ##__VA_ARGS__)

CONTRACT(enum state{
    INIT     = 0,
    RESUMED  = 1,
    CAPTURED = 2,
    FINISHED = 3,
};)

CONTRACT_GHOST({
    /* ghost state for contract check */
    caslock_t lock;
    vatomic32_t state;
    vatomic32_t pending_calls;
    vatomic32_t exiting;
    task_id running_id;
    clk_t clk;
})

CONTRACT_INIT({
    vatomic_init(&_ghost.state, INIT);
    vatomic_init(&_ghost.pending_calls, 0);
    vatomic_init(&_ghost.exiting, 0);
    caslock_init(&_ghost.lock);
    _ghost.running_id = NO_TASK;
    _ghost.clk        = 0;
})

static unsigned _actions_for(const context_t *ctx);

/*******************************************************************************
 * contract checker using ghost state
 ******************************************************************************/

CONTRACT(static void _check_plan(const context_t *ctx, plan_t p) {
    if (ctx->cat >= CAT_END_) {
        return;
    }
    /* plan.next is only set if ACTION_WAKE is given */

    ASSERT(!p.with_slack || CAT_SLACK(ctx->cat));
    switch (p.actions) {
        case ACTION_CONTINUE:
        case ACTION_RESUME:
            ASSERT(ctx->cat != CAT_CALL);
            break;
        case ACTION_CALL | ACTION_RESUME:
            ASSERT(ctx->cat == CAT_TASK_CREATE);
            break;
        case ACTION_CALL | ACTION_RETURN | ACTION_RESUME:
            ASSERT(p.replay_type != REPLAY_OFF || p.next != ctx->id);
            ASSERT(ctx->cat == CAT_CALL);
            break;
        case ACTION_RESUME | ACTION_SHUTDOWN:
            break;
        default:
            plan_print(p);
            logger_fatalf("(cat: %s) plan mismatch\n", category_str(ctx->cat));
    }
})

void
engine_init(trace_t *input, trace_t *output)
{
    logger_debugf("starting...\n");
}

int
engine_fini(const context_t *ctx, reason_t reason)
{
    CONTRACT({ //
        ASSERT(ctx->id != NO_TASK);
    })

    log(ctx, "engine_fini() called! terminating...\n");

    bool success;
    if (IS_REASON_SHUTDOWN(reason)) {
        success = true;
    } else if (IS_REASON_ABORT(reason)) {
        success = false;
    } else {
        logger_errorf("Unknown termination reason\n");
        success = false;
    }
    if (getenv("LOTTO_MODIFY_RETURN_CODE") != NULL &&
        atoi(getenv("LOTTO_MODIFY_RETURN_CODE")) == 1) {
        return success ? 0 : 240;
    }
    return !success;
}

plan_t
engine_capture(const context_t *ctx)
{
    CONTRACT({
        ASSERT(ctx->func != NULL);
        ASSERT(ctx->id != NO_TASK);
    })

    log(ctx, "CAPTURE  %s\t%s", category_str(ctx->cat), ctx->func);

    plan_t p = {
        .actions = _actions_for(ctx),
    };

    CONTRACT({
        if ((plan_has(p, ACTION_CALL) || plan_has(p, ACTION_BLOCK)) &&
            ctx->cat != CAT_TASK_CREATE) {
            vatomic_inc(&_ghost.pending_calls);
        }
        _check_plan(ctx, p);
    })

    if (plan_next(p) == ACTION_CONTINUE)
        log(ctx, "CONTINUE %s\t%s", category_str(ctx->cat), ctx->func);

    return p;
}

void
engine_resume(const context_t *ctx)
{
    log(ctx, "RESUME   %s\t%s", category_str(ctx->cat), ctx->func);

    CONTRACT({ ASSERT(ctx->id != NO_TASK); })
}

void
engine_return(const context_t *ctx)
{
    CONTRACT({
        ASSERT(ctx->id != NO_TASK);
        ASSERT(vatomic_get_dec(&_ghost.pending_calls) > 0);
    })

    log(ctx, "RETURN   %s\t%s", category_str(ctx->cat), ctx->func);
}

static inline unsigned
_actions_for(const context_t *ctx)
{
    switch (ctx->cat) {
        case CAT_TASK_CREATE:
            return ACTION_CALL | ACTION_RESUME;

        case CAT_CALL:
            return ACTION_CALL | ACTION_RETURN | ACTION_RESUME;

        case CAT_TASK_INIT:
        case CAT_TASK_FINI:

        default:
            return ACTION_RESUME;
    }
}
