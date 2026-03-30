#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // _exit

#include <vsync/atomic.h>
#define LOGGER_PREFIX LOGGER_CUR_FILE

#include <lotto/base/cappt.h>
#include <lotto/base/envvar.h>
#include <lotto/base/reason.h>
#include <lotto/base/record.h>
#include <lotto/engine/clock.h>
#include <lotto/engine/prng.h>
#include <lotto/engine/pubsub.h>
#include <lotto/engine/recorder.h>
#include <lotto/engine/sequencer.h>
#include <lotto/engine/state.h>
#include <lotto/engine/statemgr.h>
#include <lotto/runtime/capture_point.h>
#include <lotto/runtime/ingress_events.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger.h>
#include <lotto/sys/real.h>
#include <lotto/sys/stream_file.h>
#include <lotto/util/once.h>
#include <vsync/atomic/dispatch.h>
#include <vsync/spinlock/caslock.h>

#define log(cp, fmt, ...)                                                      \
    logger_debugf("[t:%lu, clk:%lu, pc:0x%lx, type:%s] " fmt "\n", cp->id,     \
                  _seq.clk, cp->pc & 0xfff, ps_type_str(cp->type_id),          \
                  ##__VA_ARGS__)

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
    type_id prev_type;
    task_id prev_task;
    task_id next_task;
    bool prev_blocking;
    bool should_record;
} sequencer_t;
static sequencer_t _seq;

clk_t clk_bound;
uint64_t time_bound_ns;

LOTTO_SUBSCRIBE(EVENT_ENGINE__AFTER_UNMARSHAL_CONFIG, {
    (void)v;
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
    _seq.prev_type     = 0;
    _seq.prev_blocking = false;
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
static unsigned _actions_for(const capture_point *cp, task_id next,
                             const event_t *e);
static bool _granularity_should_record(const capture_point *cp,
                                       const event_t *e,
                                       const struct plan *plan);
static task_id _dispatch_capture_event(const capture_point *cp,
                                       sequencer_decision *e);
void handle_creation(const capture_point *cp, sequencer_decision *e);
#ifdef LOTTO_TEST
void __attribute__((weak))
handle_creation(const capture_point *cp, sequencer_decision *e)
{
    (void)cp;
    (void)e;
}

bool sequencer_dispatch_override(const capture_point *cp, sequencer_decision *e,
                                 task_id *next);
#endif

/*******************************************************************************
 * Debug functions
 ******************************************************************************/
void __attribute__((noinline)) sequencer_clk_met();
void __attribute__((noinline)) sequencer_time_met();

/*******************************************************************************
 * Public interface
 ******************************************************************************/
struct plan
sequencer_capture(const capture_point *cp)
{
    ASSERT(cp->id != NO_TASK);

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
    sequencer_decision e = {.clk       = _seq.clk,
                            .unblocked = _seq.unblocked,
                            .replay    = ry.status != REPLAY_DONE,
                            .tset      = {}};

    /* pass capture point to handlers */
    log(cp, "sequence: cp->replay: %d", e.replay);
    capture_point capture_cp = *cp;
    capture_cp.decision      = &e;
    task_id next             = _dispatch_capture_event(&capture_cp, &e);

    replay_type_t replay_type =
        !e.replay ? REPLAY_OFF :
                    (next == ANY_TASK ? REPLAY_ANY_TASK : REPLAY_ON);

    switch (ry.status) {
        case REPLAY_LOAD:
            ASSERT(next == ANY_TASK || next == ry.id || cp->blocking);
            next = ry.id;
            break;
        case REPLAY_FORCE:
            next = ry.id;
            break;
        case REPLAY_DONE:
            break;
        case REPLAY_CONT:
            if (sequencer_config()->slack > 0 && cp->blocking) {
                next = cp->id;
            }
            break;
        default:
            ASSERT(0 && "unexpected replay status");
            break;
    }

    /* prepare plan */
    struct plan p = {
        .actions         = IS_REASON_TERMINATE(e.reason) ?
                               next != cp->id ?
                               ACTION_SHUTDOWN :
                               _actions_for(cp, next, &e) | ACTION_SHUTDOWN :
                               _actions_for(cp, next, &e),
        .clk             = _seq.clk,
        .next            = next,
        .any_task_filter = e.any_task_filter,
        .reason          = e.reason,
        .with_slack      = cp->blocking && e.replay == REPLAY_DONE,
        .replay_type     = replay_type,
    };

    if (plan_next(p) == ACTION_WAKE) {
        ASSERT(p.next != NO_TASK);
    }

    /* track decision for resume */
    _seq.should_record =
        e.should_record || _granularity_should_record(cp, &e, &p);
    _seq.next_task     = next;
    _seq.prev_type     = cp->type_id;
    _seq.prev_blocking = cp->blocking;

    /* event counting */
    {
        _seq.chpt_count += _seq.last_chpt = e.is_chpt;
        _seq.switch_count += _seq.prev_task != cp->id;
        _seq.prev_task = cp->id;
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
sequencer_resume(const capture_point *cp)
{
    bool prev_blocking = _seq.prev_blocking;
    bool prev_create   = _seq.prev_type == EVENT_TASK_CREATE;

    capture_point resume_cp = *cp;
    resume_cp.decision      = NULL;
    PS_PUBLISH(CHAIN_SEQUENCER_RESUME, cp->type_id, &resume_cp,
               (metadata_t *)&resume_cp);
    if (_seq.clk == 0)
        return;
    if (cp->id == 1 && cp->type_id == 0 && cp->type_id == 0)
        return;

    if (_seq.should_record ||
        (sequencer_config()->slack > 0 && prev_blocking &&
         cp->id != _seq.prev_task) ||
        (!prev_create && (sequencer_config()->slack == 0 || !prev_blocking) &&
         _seq.next_task != cp->id)) {
        recorder_record(cp, _seq.clk);
    }

    _update_unblocked();
}

static task_id
_dispatch_capture_event(const capture_point *cp, sequencer_decision *e)
{
#ifdef LOTTO_TEST
    task_id next = NO_TASK;
    if (sequencer_dispatch_override(cp, e, &next))
        return next;
#endif
    handle_creation(cp, e);

    capture_point capture_cp = *cp;
    capture_cp.decision      = e;
    PS_PUBLISH(CHAIN_SEQUENCER_CAPTURE, cp->type_id, &capture_cp,
               (metadata_t *)&capture_cp);

    if (!e->is_chpt) {
        ASSERT(e->next == NO_TASK || e->next == cp->id);
        return cp->id;
    }

    if (e->next != NO_TASK) {
        return e->next;
    }

    if (tidset_size(&e->tset) == 0) {
        return ANY_TASK;
    }

    if (e->selector == SELECTOR_UNDEFINED)
        e->selector = SELECTOR_RANDOM;

    switch (e->selector) {
        case SELECTOR_RANDOM:
            if (e->reason == REASON_UNKNOWN)
                e->reason = REASON_DETERMINISTIC;
            return tidset_get(&e->tset, prng_range(0, tidset_size(&e->tset)));
        case SELECTOR_FIRST:
            return tidset_get(&e->tset, 0);
        default:
            logger_debugf("Selector %d\n", e->selector);
    }

    ASSERT(0);
    return NO_TASK;
}

void
sequencer_return(const capture_point *cp)
{
    ASSERT(cp->blocking);
    _add_pending_unblocked(cp->id);
}

void
sequencer_fini(const capture_point *cp, reason_t reason)
{
    recorder_fini(_seq.clk, cp->id, reason);
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
_actions_for(const capture_point *cp, task_id next, const event_t *e)
{
    switch (cp->type_id) {
        case EVENT_TASK_CREATE:
            ASSERT(e->replay || next == ANY_TASK);
            return ACTION_BLOCK | ACTION_YIELD | ACTION_RESUME;
        case EVENT_TASK_INIT:
            ASSERT(next != NO_TASK);
            return ACTION_WAKE | ACTION_YIELD | ACTION_RESUME;
        case EVENT_TASK_FINI:
            ASSERT(next != NO_TASK);
            return ACTION_WAKE;

        default:
            if (cp->blocking) {
                ASSERT(next != NO_TASK);
                return ACTION_WAKE | ACTION_BLOCK | ACTION_RETURN |
                       ACTION_YIELD | ACTION_RESUME;
            }
#ifdef LOTTO_SEQUENCER_CONTINUE
            if (next != cp->id && cp->wait_exact)
                return ACTION_WAKE | ACTION_WAIT | ACTION_RESUME;
            else if (next != cp->id || cp->should_record)
                return ACTION_WAKE | ACTION_YIELD | ACTION_RESUME;
            else
                return ACTION_CONTINUE;
#else
            return ACTION_WAKE | ACTION_YIELD | ACTION_RESUME;
#endif
    }
}


static bool
_granularity_should_record(const capture_point *cp, const event_t *e,
                           const struct plan *plan)
{
    bool should_record = false;
    for (uint32_t i = 1; !should_record && i <= sequencer_config()->gran;
         i <<= 1) {
        if (!record_granularities_have(sequencer_config()->gran, i)) {
            continue;
        }
        switch (i) {
            case RECORD_GRANULARITY_SWITCH:
                should_record |= plan->next != cp->id;
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
