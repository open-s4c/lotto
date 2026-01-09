#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // _exit

#include <vsync/atomic.h>
#define LOG_PREFIX LOG_CUR_FILE
#define LOG_BLOCK  LOG_CUR_BLOCK

#include <lotto/base/cappt.h>
#include <lotto/base/category.h>
#include <lotto/base/context.h>
#include <lotto/base/envvar.h>
#include <lotto/base/reason.h>
#include <lotto/base/record.h>
#include <lotto/base/topic.h>
#include <lotto/brokers/pubsub.h>
#include <lotto/brokers/statemgr.h>
#include <lotto/engine/clock.h>
#include <lotto/engine/dispatcher.h>
#include <lotto/engine/prng.h>
#include <lotto/engine/recorder.h>
#include <lotto/engine/sequencer.h>
#include <lotto/states/sequencer.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger_block.h>
#include <lotto/sys/real.h>
#include <lotto/sys/stream_file.h>
#include <lotto/util/once.h>
#include <vsync/atomic/dispatch.h>
#include <vsync/spinlock/caslock.h>

#define log(ctx, fmt, ...)                                                     \
    logger_debugf("[t:%lu, clk:%lu, pc:0x%lx] " fmt "\n", ctx->id, _seq.clk,      \
               ctx->pc & 0xfff, ##__VA_ARGS__)

typedef struct {
    _Atomic(clk_t) clk;

    tidset_t unblocked;
    struct {
        tidset_t unblocked;
        caslock_t lock;
    } pending;

    uint64_t chpt_count;
    uint64_t switch_count;
    bool last_chpt;
    category_t prev_cat;

    task_id prev_task;
    task_id next_task;
    bool should_record;
} sequencer_t;
static sequencer_t _seq;

clk_t clk_bound;
uint64_t time_bound_ns;

PS_SUBSCRIBE_INTERFACE(TOPIC_AFTER_UNMARSHAL_CONFIG, {
    const char *var;
    if ((var = getenv("LOTTO_RECORD_GRANULARITY"))) {
        record_granularities_t gran;
        gran                     = record_granularities_from(var);
        sequencer_config()->gran = gran;
    }
})

void
sequencer_reset(void)
{
    tidset_init(&_seq.unblocked);
    tidset_init(&_seq.pending.unblocked);
    caslock_init(&_seq.pending.lock);
    _seq.clk           = 0;
    _seq.chpt_count    = 0;
    _seq.switch_count  = 0;
    _seq.should_record = false;
    _seq.prev_task     = NO_TASK;
    _seq.next_task     = NO_TASK;
    const char *var    = getenv("LOTTO_DEBUG_CLK_BOUND");
    if (var) {
        clk_bound = atoll(var);
    }
    var = getenv("LOTTO_DEBUG_TIME_BOUND_NS");
    if (var) {
        time_bound_ns = atoll(var);
    }
}
REGISTER_EPHEMERAL(_seq, ({ sequencer_reset(); }))

/*******************************************************************************
 * Private runtime interface
 ******************************************************************************/
static void _update_unblocked(void);
static void _add_pending_unblocked(task_id id);
static unsigned _actions_for(const context_t *ctx, task_id next,
                             const event_t *e);
static bool _granularity_should_record(const context_t *ctx, const event_t *e,
                                       const plan_t *plan);

/*******************************************************************************
 * Debug functions
 ******************************************************************************/
void __attribute__((noinline)) sequencer_clk_met();
void __attribute__((noinline)) sequencer_time_met();

/*******************************************************************************
 * Public interface
 ******************************************************************************/
plan_t
sequencer_capture(const context_t *ctx)
{
    ASSERT(ctx->id != NO_TASK);

    _seq.clk++;
#ifdef QLOTTO_ENABLED
    if (clk_bound && _seq.clk >= clk_bound) {
        sequencer_clk_met();
    }

    if (time_bound_ns && clock_ns() >= time_bound_ns) {
        sequencer_time_met();
    }
#endif
    /* try replaying any record from input trace */
    replay_t ry = recorder_replay(_seq.clk);

    /* prepare determinants */
    _update_unblocked();

    /* prepare capture point */
    event_t e = {.clk       = _seq.clk,
                 .unblocked = _seq.unblocked,
                 .replay    = ry.status != REPLAY_DONE,
                 .tset      = {}};

    /* pass capture point to handlers */
    log(ctx, "sequence: cp->replay: %d", e.replay);
    task_id next = dispatch_event(ctx, &e);

    replay_type_t replay_type =
        !e.replay ? REPLAY_OFF :
                    (next == ANY_TASK ? REPLAY_ANY_TASK : REPLAY_ON);

    switch (ry.status) {
        case REPLAY_LOAD:
            ASSERT(next == ANY_TASK || next == ry.id || CAT_BLOCK(ctx->cat));
            next = ry.id;
            break;
        case REPLAY_FORCE:
            next = ry.id;
            break;
        case REPLAY_DONE:
            break;
        case REPLAY_CONT:
            if (sequencer_config()->slack > 0 && CAT_SLACK(ctx->cat)) {
                next = ctx->id;
            }
            break;
        default:
            ASSERT(0 && "unexpected replay status");
            break;
    }

    /* prepare plan */
    plan_t p = {
        .actions         = IS_REASON_TERMINATE(e.reason) ?
                               next != ctx->id ?
                               ACTION_SHUTDOWN :
                               _actions_for(ctx, next, &e) | ACTION_SHUTDOWN :
                               _actions_for(ctx, next, &e),
        .clk             = _seq.clk,
        .next            = next,
        .any_task_filter = e.any_task_filter,
        .reason          = e.reason,
        .with_slack      = CAT_SLACK(ctx->cat) && e.replay == REPLAY_DONE,
        .replay_type     = replay_type,
    };

    if (plan_next(p) == ACTION_WAKE) {
        ASSERT(p.next != NO_TASK);
    }

    /* track decision for resume */
    _seq.should_record =
        e.should_record || _granularity_should_record(ctx, &e, &p);
    _seq.next_task = next;
    _seq.prev_cat  = ctx->cat;

    /* event counting */
    {
        _seq.chpt_count += _seq.last_chpt = e.is_chpt;
        _seq.switch_count += _seq.prev_task != ctx->id;
        _seq.prev_task = ctx->id;
    }

    /* clean up */
    tidset_clear(&_seq.unblocked);

    return p;
}

/**
 * Recording must happen in either of the following cases:
 *
 * - sequencer requested to record
 * - when slack is enabled and a blocking cat resulted in a different task
 * - when a category other than create or blocking deviated from the sequencer
 * decision
 * - when slack is disabled and a blocking category deviated from the sequencer
 * decision
 */
void
sequencer_resume(const context_t *ctx)
{
    struct value val = any(ctx);
    PS_PUBLISH_INTERFACE(TOPIC_NEXT_TASK, val);
    if (ctx->id == 1 && ctx->cat == CAT_NONE) {
        recorder_config();
        return;
    }

    if (_seq.should_record ||
        (sequencer_config()->slack > 0 && CAT_SLACK(_seq.prev_cat) &&
         ctx->id != _seq.prev_task) ||
        (_seq.prev_cat != CAT_TASK_CREATE &&
         (sequencer_config()->slack == 0 || !CAT_SLACK(_seq.prev_cat)) &&
         _seq.next_task != ctx->id)) {
        recorder_record(ctx, _seq.clk);
    }

    _update_unblocked();
}

void
sequencer_return(const context_t *ctx)
{
    ASSERT(CAT_BLOCK(ctx->cat));
    _add_pending_unblocked(ctx->id);
}

void
sequencer_fini(const context_t *ctx, reason_t reason)
{
    recorder_fini(_seq.clk, ctx->id, reason);
    logger_debugf("[lotto] chpts: %lu, switches: %lu, clks: %lu\n",
               _seq.chpt_count, _seq.switch_count, _seq.clk);
}

clk_t
sequencer_get_clk()
{
    return _seq.clk;
}

/*******************************************************************************
 * internal functions
 ******************************************************************************/
static inline unsigned
_actions_for(const context_t *ctx, task_id next, const event_t *e)
{
    switch (ctx->cat) {
        case CAT_TASK_CREATE:
            ASSERT(e->replay || next == ANY_TASK);
            return ACTION_CALL | ACTION_YIELD | ACTION_RESUME;

        case CAT_CALL:
            ASSERT(next != NO_TASK);
            return ACTION_WAKE | ACTION_CALL | ACTION_RETURN | ACTION_YIELD |
                   ACTION_RESUME;

        case CAT_TASK_BLOCK:
            ASSERT(next != NO_TASK);
            return ACTION_WAKE | ACTION_BLOCK | ACTION_RETURN | ACTION_YIELD |
                   ACTION_RESUME;

        case CAT_TASK_INIT:
            ASSERT(next != NO_TASK);
            return ACTION_WAKE | ACTION_YIELD | ACTION_RESUME;

        case CAT_TASK_FINI:
            ASSERT(next != NO_TASK);
            return ACTION_WAKE;

        default:
#ifdef LOTTO_SEQUENCER_CONTINUE
            if (next != ctx->id && cp->wait_exact)
                return ACTION_WAKE | ACTION_WAIT | ACTION_RESUME;
            else if (next != ctx->id || cp->should_record)
                return ACTION_WAKE | ACTION_YIELD | ACTION_RESUME;
            else
                return ACTION_CONTINUE;
#else
            return ACTION_WAKE | ACTION_YIELD | ACTION_RESUME;
#endif
    }
}


static bool
_granularity_should_record(const context_t *ctx, const event_t *e,
                           const plan_t *plan)
{
    bool should_record = false;
    for (uint32_t i = 1; !should_record && i <= sequencer_config()->gran;
         i <<= 1) {
        if (!record_granularities_have(sequencer_config()->gran, i)) {
            continue;
        }
        switch (i) {
            case RECORD_GRANULARITY_SWITCH:
                should_record |= plan->next != ctx->id;
                break;
            case RECORD_GRANULARITY_CHPT:
                should_record |= e->is_chpt;
                break;
            case RECORD_GRANULARITY_CAPTURE:
                should_record = true;
                break;
            default:
                ASSERT(0 && "unknown granularity");
        }
    }
    return should_record;
}

static void
_update_unblocked()
{
    caslock_acquire(&_seq.pending.lock);
    for (size_t i = 0; i < tidset_size(&_seq.pending.unblocked); i++) {
        task_id id = tidset_get(&_seq.pending.unblocked, i);
        bool ok    = tidset_insert(&_seq.unblocked, id);
        ASSERT(ok && "inserting id of again");
    }
    tidset_clear(&_seq.pending.unblocked);
    caslock_release(&_seq.pending.lock);
}
static void
_add_pending_unblocked(task_id id)
{
    caslock_acquire(&_seq.pending.lock);
    if (!tidset_insert(&_seq.pending.unblocked, id))
        ASSERT(0 && "inserting id again");
    caslock_release(&_seq.pending.lock);
}

void __attribute__((noinline))
sequencer_clk_met()
{
    logger_debugf("clk met\n");
    clk_bound = 0;
}

void __attribute__((noinline))
sequencer_time_met()
{
    logger_debugf("time met\n");
    time_bound_ns = 0;
}

void
sequencer_set_clk_bound(clk_t value)
{
    clk_bound = value;
}

void
sequencer_set_time_bound_ns(uint64_t value)
{
    time_bound_ns = value;
}
