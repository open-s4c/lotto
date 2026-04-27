#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // _exit

#include <dice/chains/capture.h>
#include <lotto/base/reason.h>
#include <lotto/engine/pubsub.h>
#include <lotto/engine/recorder.h>
#include <lotto/engine/sequencer.h>
#include <lotto/engine/statemgr.h>
#include <lotto/runtime/events.h>
#include <lotto/sys/logger.h>
#include <lotto/sys/now.h>
#include <lotto/util/contract.h>
#include <lotto/util/once.h>
#include <vsync/atomic.h>
#include <vsync/atomic/dispatch.h>
#include <vsync/spinlock/caslock.h>

#ifdef log
    #error
#endif
#define log(cp, fmt, ...)                                                      \
    logger_debugf("[t:%lu, " CONTRACT("clk:%lu, ") "pc:0x%lx, type:%s] " fmt   \
                                                   "\n",                       \
                  cp->id, CONTRACT(_ghost.clk, ) cp->pc & 0xfff,               \
                  ps_type_str(cp->type_id), ##__VA_ARGS__)


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
    vatomic32_t pending_blocking_returns;
    vatomic32_t exiting;
    task_id running_id;
    clk_t clk;
})

CONTRACT_INIT({
    vatomic_init(&_ghost.state, INIT);
    vatomic_init(&_ghost.pending_blocking_returns, 0);
    vatomic_init(&_ghost.exiting, 0);
    caslock_init(&_ghost.lock);
    _ghost.running_id = NO_TASK;
    _ghost.clk        = 0;
})

static inline bool
_engine_is_ingress_chain(chain_id chain)
{
    switch (chain) {
        case CHAIN_INGRESS_EVENT:
        case CHAIN_INGRESS_BEFORE:
        case CHAIN_INGRESS_AFTER:
            return true;
        default:
            return false;
    }
}

LOTTO_ADVERTISE_TYPE(EVENT_ENGINE__START)

/*******************************************************************************
 * contract checker using ghost state
 ******************************************************************************/

CONTRACT(static void _check_plan(const capture_point *cp, struct plan p) {
    /* plan.next is only set if ACTION_WAKE is given */

    ASSERT(p.next != NO_TASK);
    ASSERT(!p.with_slack || cp->blocking);
    unsigned pure_actions =
        p.actions & ACTION_SHUTDOWN && p.actions != ACTION_SHUTDOWN ?
            p.actions & ~ACTION_SHUTDOWN :
            p.actions;
    switch (pure_actions) {
        case ACTION_CONTINUE:
            // fallthru
        case ACTION_WAKE | ACTION_YIELD | ACTION_RESUME:
            ASSERT(cp->type_id != EVENT_TASK_FINI);
            break;
        case ACTION_BLOCK | ACTION_YIELD | ACTION_RESUME:
            ASSERT(cp->type_id == EVENT_TASK_CREATE);
            break;
        case ACTION_WAKE | ACTION_BLOCK | ACTION_RETURN | ACTION_YIELD |
            ACTION_RESUME:
            ASSERT(p.replay_type != REPLAY_OFF || p.next != cp->id);
            ASSERT(cp->blocking);
            break;
        case ACTION_WAKE:
            ASSERT(p.next != cp->id);
            ASSERT(cp->type_id == EVENT_TASK_FINI);
            break;
        case ACTION_SHUTDOWN:
            break;
        default:
            plan_print(p);
            logger_fatalf("(cat: %s, type:%u, src:%u) plan mismatch\n",
                          ps_type_str(cp->type_id), cp->type_id, cp->type_id);
    }
})

void
engine_init(trace_t *input, trace_t *output)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    logger_debugf("starting...\n");
    LOTTO_PUBLISH(EVENT_ENGINE__START, nil);
    recorder_init(input, output);
}

int
engine_fini(const capture_point *cp, reason_t reason)
{
    CONTRACT({ //
        ASSERT(vatomic_xchg(&_ghost.state, FINISHED) != FINISHED);
        ASSERT(cp->id != NO_TASK);
    })

    log(cp, "engine_fini() called! terminating...\n");
    sequencer_fini(cp, reason);

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

struct plan
engine_capture(const capture_point *cp)
{
    ASSERT(_engine_is_ingress_chain(cp->chain_id));
    CONTRACT({
        ASSERT(vatomic_read(&_ghost.state) != FINISHED);
        ASSERT(cp->func != NULL);
        ASSERT(cp->id != NO_TASK);

        _ghost.clk++;

        ASSERT(vatomic_read(&_ghost.state) == RESUMED);
        ASSERT(caslock_tryacquire(&_ghost.lock));
    })

    log(cp, "CAPTURE  %s\t%s", ps_type_str(cp->type_id), cp->func);

    struct plan p = sequencer_capture(cp);

    CONTRACT({
        ASSERT(p.clk == _ghost.clk);

        if (plan_next(p) != ACTION_CONTINUE) {
            /* ACTION_CONTINUE: the plan is basically empty, there is no
             * reason to call engine_resume.
             */
            vatomic_write(&_ghost.state, CAPTURED);
        }

        if (plan_has(p, ACTION_BLOCK) && cp->type_id != EVENT_TASK_CREATE) {
            vatomic_inc(&_ghost.pending_blocking_returns);
        }

        ASSERT(_ghost.running_id == cp->id);

        caslock_release(&_ghost.lock);
        _check_plan(cp, p);
    })

    if (plan_next(p) == ACTION_CONTINUE)
        log(cp, "CONTINUE %s\t%s", ps_type_str(cp->type_id), cp->func);

    return p;
}

void
engine_resume(const capture_point *cp)
{
    ASSERT(_engine_is_ingress_chain(cp->chain_id));
    log(cp, "RESUME   %s\t%s", ps_type_str(cp->type_id), cp->func);

    CONTRACT({
        ASSERT(caslock_tryacquire(&_ghost.lock));
        enum state old_state = vatomic_xchg(&_ghost.state, RESUMED);
        ASSERT(old_state != FINISHED);
        ASSERT(old_state == INIT || old_state == CAPTURED);
        ASSERT(cp->id != NO_TASK);
        _ghost.running_id = cp->id;
    })

    sequencer_resume(cp);

    CONTRACT(caslock_release(&_ghost.lock));
}

void
engine_return(const capture_point *cp)
{
    ASSERT(_engine_is_ingress_chain(cp->chain_id));
    CONTRACT({
        ASSERT(cp->id != NO_TASK);
        if (vatomic_read(&_ghost.state) == FINISHED) {
            logger_warnf("ignoring engine_return after fini (t: %lu)\n",
                         cp->id);
            return;
        }
        ASSERT(vatomic_get_dec(&_ghost.pending_blocking_returns) > 0);
    })

    log(cp, "RETURN   %s\t%s", ps_type_str(cp->type_id), cp->func);
    sequencer_return(cp);
}
