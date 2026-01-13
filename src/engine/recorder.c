/*
 */

/**
 * The recorder glues together the input and output traces and the statemgr,
 *making the decision of what and when to replay something, and taking the
 *record commands from the caller (sequencer)
 **/

#define LOGGER_PREFIX LOGGER_CUR_FILE
#define LOGGER_BLOCK  LOGGER_CUR_BLOCK

#include <lotto/base/context.h>
#include <lotto/base/envvar.h>
#include <lotto/base/trace.h>
#include <lotto/brokers/pubsub.h>
#include <lotto/brokers/statemgr.h>
#include <lotto/engine/recorder.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger_block.h>
#include <lotto/sys/stdio.h>
#include <lotto/util/contract.h>
#include <lotto/util/once.h>

#define IS_AFTER_KIND(x) (((x) & (RECORD_START | RECORD_CONFIG)) != 0)

void __attribute__((noinline)) recorder_end_trace()
{
    logger_debugf("trace fully loaded\n");
    // nothing happends here
}

void __attribute__((noinline)) recorder_end_replay()
{
    logger_debugf("end of replay\n");
    PS_PUBLISH_INTERFACE(TOPIC_REPLAY_END, nil);
}

static struct {
    trace_t *input;
    trace_t *output;
    record_t *finalr;
} _recorder;

void
recorder_init(trace_t *input, trace_t *output)
{
    logger_debugf("recorder_init called\n");
    _recorder.input  = input;
    _recorder.output = output;
}

CONTRACT_GHOST({
    clk_t record_clk;
    clk_t replay_clk;
    bool finito;
})

CONTRACT_SUBSCRIBE(TOPIC_ENGINE_START, {
    _ghost.record_clk = 0;
    _ghost.replay_clk = 0;
    _ghost.finito     = false;
})

static void LOTTO_CONSTRUCTOR
_init_finalr()
{
    /* this has to be called after the constructors to find the right size of
     * the FINAL record. */
    _recorder.finalr = record_alloc(statemgr_size(STATE_TYPE_FINAL));
}

static void
_recorder_out(record_t *r)
{
    if (!_recorder.output)
        return;
    ASSERT(r->kind != RECORD_NONE);
    int ok = trace_append(_recorder.output, r);
    ASSERT(ok == TRACE_OK);
}

static void
_recorder_out_safe(record_t *r)
{
    if (!_recorder.output)
        return;
    ASSERT(r->kind != RECORD_NONE);
    int ok = trace_append_safe(_recorder.output, r);
    ASSERT(ok == TRACE_OK);
}

static void
_recorder_out_clone(const record_t *r)
{
    if (!_recorder.output)
        return;
    ASSERT(r->kind != RECORD_NONE);
    int ok = trace_append(_recorder.output, record_clone(r));
    ASSERT(ok == TRACE_OK);
}

replay_t
_recorder_replay_next(clk_t clk)
{
    replay_t ry = {.status = REPLAY_DONE, .id = NO_TASK};

    while (1) {
        record_t *r = trace_next(_recorder.input, RECORD_ANY);
        if (r == NULL) {
            recorder_end_trace();
            _recorder.input = NULL;
            break;
        }

        /* record must be at most 1 tick behind */
        ASSERT(clk == 0 || r->clk >= clk - 1);

        /* if we are to unmarshal/process START and CONFIG, we **must**
         * be at the end of the clock, so r->clk+1 == clk.
         * for normal records we load them at the start of the clock, so
         * they must match 1 to 1 */
        if (r->clk > clk || (IS_AFTER_KIND(r->kind) && r->clk == clk)) {
            if (ry.status == REPLAY_DONE) {
                /* should load later */
                ry.status = REPLAY_CONT;
            }
            break;
        }

        logger_debugf("replaying record [clk: %lu kind: %s]\n", r->clk,
                   kind_str(r->kind));
        /* we match the record, let's load it. */
        statemgr_record_unmarshal(r);
        switch (r->kind) {
            case RECORD_INFO: {
                struct value val = any(r);
                PS_PUBLISH_INTERFACE(TOPIC_INFO_RECORD_LOAD, val);
                break;
            }
            case RECORD_FORCE:
                ASSERT(ry.status == REPLAY_DONE);
                ry = (replay_t){.status = REPLAY_FORCE, .id = r->id};
                _recorder_out_clone(r);
                break;
            case RECORD_SCHED:
                ry.id = r->id;
                if (ry.status == REPLAY_DONE) {
                    ry.status = REPLAY_LOAD;
                }
                break;
            case RECORD_OPAQUE:
                break;
            case RECORD_EXIT:
                logger_warnf("ignoring EXIT record (clk: %lu)\n", clk);
                recorder_end_trace();
                break;

                /* leftovers from previous clock */
            case RECORD_START:
                ASSERT(clk == 1);
                // fallthru
            case RECORD_CONFIG:
                _recorder_out_clone(r);
                break;
            default:
                logger_fatalf("unexpected %s record (clk: %lu)\n",
                           kind_str(r->kind), clk);
        }
        trace_advance(_recorder.input);
    }
    return ry;
}

replay_t
recorder_replay(clk_t clk)
{
    CONTRACT({
        ASSERT(!_ghost.finito && "do not call recorder after fini");
        ASSERT(clk == ++_ghost.replay_clk &&
               "clock contiguous and starting from 1");
    })

    logger_debugf("recorder_replay called (clk: %lu)\n", clk);
    replay_t ry = {.status = REPLAY_DONE, .id = NO_TASK};
    if (!_recorder.input) {
        return ry;
    }

    return _recorder_replay_next(clk);
}

void
recorder_record(const context_t *ctx, clk_t clk)
{
    CONTRACT({
        ASSERT(!_ghost.finito && "do not call recorder after fini");
        ASSERT(ctx->id != NO_TASK && "do not record for NO_TASK");
        ASSERT(clk > 0 && "do not record at clock 0");
        ASSERT(clk > _ghost.record_clk && "clk strict monotonic");
        _ghost.record_clk = clk;
        ASSERT(clk >= _ghost.replay_clk && "record clk >= to replay");
    })

    logger_debugf("recorder_record called (clk: %lu id: %lu)\n", clk, ctx->id);

    if (!_recorder.output)
        return;

    record_t *r = record_alloc(statemgr_size(STATE_TYPE_PERSISTENT));
    ASSERT(r != NULL);
    r->kind = RECORD_SCHED;
    r->id   = ctx->id;
    r->cat  = ctx->cat;
    r->pc   = ctx->pc;
    r->clk  = clk;
    statemgr_marshal(&r->data, STATE_TYPE_PERSISTENT);
    _recorder_out(r);

    /* if the replay is over, input is NULL. The following call to record
     * marks the start of the recording phase (ie, the end of the replay).
     */
    if (_recorder.input == NULL)
        once(recorder_end_replay());
}

PS_SUBSCRIBE_INTERFACE(TOPIC_INFO_RECORD_SAVE, {
    const record_t *r = (const record_t *)as_any(v);
    ASSERT(r->kind == RECORD_INFO);
    _recorder_out_clone(r);
})

void
recorder_fini(clk_t clk, task_id id, reason_t reason)
{
    CONTRACT({
        ASSERT(!_ghost.finito && "do not call recorder after fini");
        _ghost.finito = true;
    })

    if (!_recorder.output)
        return;

    if (!_recorder.finalr) {
        logger_warnf("no record preallocated, cannot create trace\n");
        return;
    }

    record_t *r = NULL;

    if (IS_REASON_RECORD_FINAL(reason)) {
        /* We are not in a signal handler, so it's safe to record
         * FINAL states. */
        r = record_alloc(statemgr_size(STATE_TYPE_FINAL));
        ASSERT(r);
        statemgr_marshal(r->data, STATE_TYPE_FINAL);
    } else {
        r = _recorder.finalr;
        ASSERT(r);
        statemgr_marshal(r->data, STATE_TYPE_EMPTY);
    }

    r->kind   = RECORD_EXIT;
    r->clk    = clk;
    r->id     = id;
    r->reason = reason;
    _recorder_out_safe(r);
}
