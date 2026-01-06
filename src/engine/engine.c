#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // _exit

#include <vsync/atomic.h>
#include <vsync/atomic/dispatch.h>
#include <vsync/spinlock/caslock.h>

#define LOG_PREFIX LOG_CUR_FILE
#define LOG_BLOCK  LOG_CUR_BLOCK
#include <lotto/base/reason.h>
#include <lotto/brokers/catmgr.h>
#include <lotto/brokers/pubsub.h>
#include <lotto/brokers/statemgr.h>
#include <lotto/engine/engine.h>
#include <lotto/engine/recorder.h>
#include <lotto/engine/sequencer.h>
#include <lotto/sys/logger_block.h>
#include <lotto/sys/now.h>
#include <lotto/util/contract.h>
#include <lotto/util/once.h>

#define log(ctx, fmt, ...)                                                     \
    log_debugf("[t:%lu, " CONTRACT("clk:%lu, ") "pc:0x%lx] " fmt "\n",         \
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

/*******************************************************************************
 * contract checker using ghost state
 ******************************************************************************/

CONTRACT(static void _check_plan(const context_t *ctx, plan_t p) {
    if (ctx->cat >= CAT_END_) {
        return;
    }
    /* plan.next is only set if ACTION_WAKE is given */

    ASSERT(p.next != NO_TASK);
    ASSERT(!p.with_slack || CAT_SLACK(ctx->cat));
    unsigned pure_actions =
        p.actions & ACTION_SHUTDOWN && p.actions != ACTION_SHUTDOWN ?
            p.actions & ~ACTION_SHUTDOWN :
            p.actions;
    switch (pure_actions) {
        case ACTION_CONTINUE:
        case ACTION_WAKE | ACTION_YIELD | ACTION_RESUME:
            ASSERT(p.any_task_filter == NULL || CAT_WAIT(ctx->cat) ||
                   ctx->cat >= CAT_END_);
            ASSERT(ctx->cat != CAT_TASK_FINI);
            break;
        case ACTION_CALL | ACTION_YIELD | ACTION_RESUME:
            ASSERT(ctx->cat == CAT_TASK_CREATE);
            break;
        case ACTION_WAKE | ACTION_CALL | ACTION_RETURN | ACTION_YIELD |
            ACTION_RESUME:
            ASSERT(p.replay_type != REPLAY_OFF || p.next != ctx->id);
            ASSERT(ctx->cat == CAT_CALL);
            break;
        case ACTION_WAKE | ACTION_BLOCK | ACTION_RETURN | ACTION_YIELD |
            ACTION_RESUME:
            ASSERT(p.replay_type != REPLAY_OFF || p.next != ctx->id);
            ASSERT(ctx->cat == CAT_TASK_BLOCK);
            break;
        case ACTION_WAKE:
            ASSERT(p.next != ctx->id);
            ASSERT(ctx->cat == CAT_TASK_FINI);
            break;
        case ACTION_SHUTDOWN:
            break;
        default:
            plan_print(p);
            log_fatalf("(cat: %s) plan mismatch\n", category_str(ctx->cat));
    }
})

void
engine_init(trace_t *input, trace_t *output)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    log_debugf("starting...\n");
    PS_PUBLISH_INTERFACE(TOPIC_ENGINE_START, nil);
    recorder_init(input, output);
}

int
engine_fini(const context_t *ctx, reason_t reason)
{
    CONTRACT({ //
        ASSERT(vatomic_xchg(&_ghost.state, FINISHED) != FINISHED);
        ASSERT(ctx->id != NO_TASK);
    })

    log(ctx, "engine_fini() called! terminating...\n");
    sequencer_fini(ctx, reason);

    bool success;
    if (IS_REASON_SHUTDOWN(reason)) {
        success = true;
    } else if (IS_REASON_ABORT(reason)) {
        success = false;
    } else {
        log_errorf("Unknown termination reason\n");
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
        ASSERT(vatomic_read(&_ghost.state) != FINISHED);
        ASSERT(ctx->func != NULL);
        ASSERT(ctx->id != NO_TASK);

        _ghost.clk++;

        ASSERT(vatomic_read(&_ghost.state) == RESUMED);
        ASSERT(caslock_tryacquire(&_ghost.lock));
    })

    log(ctx, "CAPTURE  %s\t%s", category_str(ctx->cat), ctx->func);

    struct value val = any(ctx);
    PS_PUBLISH_INTERFACE(TOPIC_BEFORE_CAPTURE, val);
    plan_t p = sequencer_capture(ctx);

    CONTRACT({
        ASSERT(p.clk == _ghost.clk);

        if (plan_next(p) != ACTION_CONTINUE) {
            /* ACTION_CONTINUE: the plan is basically empty, there is no reason
             * to call engine_resume.
             */
            vatomic_write(&_ghost.state, CAPTURED);
        }

        if ((plan_has(p, ACTION_CALL) || plan_has(p, ACTION_BLOCK)) &&
            ctx->cat != CAT_TASK_CREATE) {
            vatomic_inc(&_ghost.pending_calls);
        }

        ASSERT(_ghost.running_id == ctx->id);

        caslock_release(&_ghost.lock);
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

    CONTRACT({
        ASSERT(caslock_tryacquire(&_ghost.lock));
        enum state old_state = vatomic_xchg(&_ghost.state, RESUMED);
        ASSERT(old_state != FINISHED);
        ASSERT(old_state == INIT || old_state == CAPTURED);
        ASSERT(ctx->id != NO_TASK);
        _ghost.running_id = ctx->id;
    })

    sequencer_resume(ctx);

    CONTRACT(caslock_release(&_ghost.lock));
}

void
engine_return(const context_t *ctx)
{
    CONTRACT({
        ASSERT(ctx->id != NO_TASK);
        if (vatomic_read(&_ghost.state) == FINISHED) {
            log_warnf("ignoring engine_return after fini (t: %lu)\n", ctx->id);
            return;
        }
        ASSERT(vatomic_get_dec(&_ghost.pending_calls) > 0);
    })

    log(ctx, "RETURN   %s\t%s", category_str(ctx->cat), ctx->func);
    sequencer_return(ctx);
}
