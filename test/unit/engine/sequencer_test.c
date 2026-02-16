#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <lotto/base/cappt.h>
#include <lotto/base/context.h>
#include <lotto/base/trace_flat.h>
#include <lotto/brokers/pubsub.h>
#include <lotto/brokers/statemgr.h>
#include <lotto/engine/dispatcher.h>
#include <lotto/engine/recorder.h>
#include <lotto/engine/sequencer.h>

// NOLINTBEGIN(bugprone-suspicious-memory-comparison)

#define new_ctx(...)                                                           \
    (&(context_t){.func = "UNKNOWN",                                           \
                  .cat  = CAT_NONE,                                            \
                  .id   = NO_TASK,                                             \
                  .args = {0},                                                 \
                  __VA_ARGS__})
void sequencer_reset(void);

/*******************************************************************************
 * mock
 ******************************************************************************/
typedef struct {
    int exit_err;
    task_id wake_id;
    task_id yield_id;
    context_t *ctx;
    bool skip;
} expect_t;

typedef struct {
    task_id next;
    expect_t expect;
    nanosec_t now;
    bool marshaled;
    bool unmarshaled;
    bool config_marshaled;
    bool config_unmarshaled;
} mock_t;
static mock_t mock;

task_id
dispatch_event(const context_t *ctx, event_t *e)
{
    assert(mock.expect.ctx);
    assert(memcmp(mock.expect.ctx, ctx, sizeof(context_t)) == 0);
    assert(mock.expect.skip == e->skip);

    free(mock.expect.ctx);
    mock.expect.ctx  = NULL;
    mock.expect.skip = false;

    return mock.next;
}
void
scheduler_postprocess(const context_t *ctx)
{
    assert(mock.expect.ctx);
    assert(memcmp(mock.expect.ctx, ctx, sizeof(context_t)) == 0);

    free(mock.expect.ctx);
    mock.expect.ctx = NULL;
}
void
expect_process(const context_t *ctx, bool skip)
{
    if (mock.expect.ctx != NULL)
        free(mock.expect.ctx);
    mock.expect.ctx  = (context_t *)malloc(sizeof(context_t));
    *mock.expect.ctx = *ctx;
    mock.expect.skip = skip;
}
bool
expect_processed()
{
    return mock.expect.ctx == NULL;
}

size_t
statemgr_size(state_type_t type)
{
    (void)type;
    return 0;
}

void *
statemgr_marshal(void *buf, state_type_t type)
{
    (void)type;
    mock.marshaled = true;
    return buf;
}
bool
expect_marshaled()
{
    bool v         = mock.marshaled;
    mock.marshaled = false;
    return v;
}

void
statemgr_record_unmarshal(const record_t *r)
{
    mock.unmarshaled = true;
}
bool
expect_unmarshaled()
{
    bool v           = mock.unmarshaled;
    mock.unmarshaled = false;
    return v;
}

int
clock_gettime(clockid_t clk_id, struct timespec *tp)
{
    logger_printf("read clock: %.5f\n", in_sec(mock.now));
    tp->tv_sec  = (long)(mock.now / NOW_SECOND);
    tp->tv_nsec = (long)(mock.now % NOW_SECOND);

    return 0;
}

size_t
config_size()
{
    return 0;
}

void *
config_marshal(void *buf)
{
    mock.config_marshaled = true;
    return buf;
}

bool
expect_config_marshaled()
{
    bool v                = mock.config_marshaled;
    mock.config_marshaled = false;
    return v;
}

const void *
config_unmarshal(const void *buf)
{
    mock.config_unmarshaled = true;
    return buf;
}

bool
expect_config_unmarshaled()
{
    bool v                  = mock.config_unmarshaled;
    mock.config_unmarshaled = false;
    return v;
}

void
config_init()
{
}

void
statemgr_register(marshable_t *m, state_type_t type)
{
    (void)type;
    (void)m;
}

void
initializer_init()
{
}

void
ichpt_load(stream_t *s, bool reset)
{
}

void
ichpt_save(stream_t *s)
{
}

void
engine_get_cur_ns()
{
}

/*******************************************************************************
 * Test main task initialization
 ******************************************************************************/
void
test_main_task()
{
    PS_PUBLISH_INTERFACE(TOPIC_ENGINE_START, nil);
    printf("Test: %s\n", __FUNCTION__);
    task_id tid = 1;

    context_t *ctx = new_ctx(.id = tid, .cat = CAT_TASK_INIT);
    expect_process(ctx, false);
    mock.next  = tid;
    mock.now   = 5 * NOW_SECOND;
    plan_t act = sequencer_capture(ctx);
    assert(act.next == tid);
    // assert(act.reason == REASON_CALL);
    assert(act.actions == (ACTION_WAKE | ACTION_YIELD | ACTION_RESUME));
}

/*******************************************************************************
 * Test replay
 ******************************************************************************/
static void
test_replay()
{
    PS_PUBLISH_INTERFACE(TOPIC_ENGINE_START, nil);
    logger(LOGGER_DEBUG, stderr);
    printf("Test: %s\n", __FUNCTION__);
    task_id tid          = 1;
    task_id other        = 2;
    record_t *r          = NULL;
    context_t *ctx_tid   = NULL;
    context_t *ctx_other = NULL;
    plan_t plan          = {0};

    trace_t *t = trace_flat_create(NULL);

    sequencer_reset();
    recorder_init(t, NULL);
    // struct value val = flag_uval(3);
    // PS_PUBLISH_INTERFACE(FLAG_CLI_REPLAY_GOAL, val);

    /* at capture point 2, continue with tid */
    r         = record_alloc(0);
    r->kind   = RECORD_SCHED;
    r->id     = tid;
    r->reason = REASON_WATCHDOG;
    r->clk    = 2;
    trace_append(t, r);

    /* at capture point 3, replay asks to schedule another task */
    r         = record_alloc(0);
    r->kind   = RECORD_SCHED;
    r->reason = REASON_DETERMINISTIC;
    r->id     = other; /* <-- this is the other task */
    r->clk    = 3;
    trace_append(t, r);

    /* capture point 1 */
    ctx_tid = new_ctx(.id = tid, .cat = CAT_TASK_INIT);
    expect_process(ctx_tid, false);
    plan = sequencer_capture(ctx_tid);
    assert(plan.next == tid);
    sequencer_resume(ctx_tid);
    assert(plan.actions == (ACTION_WAKE | ACTION_YIELD | ACTION_RESUME));
    assert(expect_processed());
    assert(!expect_unmarshaled());
    assert(!expect_marshaled());

    /* capture point 2 */
    ctx_tid = new_ctx(.id = tid, .cat = CAT_BEFORE_WRITE);
    expect_process(ctx_tid, false);
    mock.next = ANY_TASK;
    plan      = sequencer_capture(ctx_tid);
    assert(plan.next == tid);
    sequencer_resume(ctx_tid);
    assert(plan.actions == (ACTION_WAKE | ACTION_YIELD | ACTION_RESUME));
    assert(trace_last(t) != NULL);
    assert(trace_last(t) == trace_next(t, RECORD_ANY));
    assert(expect_processed());
    assert(expect_unmarshaled());
    assert(!expect_marshaled());

    /* capture point 3 */
    ctx_tid = new_ctx(.id = tid, .cat = CAT_BEFORE_AREAD);
    expect_process(ctx_tid, false);
    plan = sequencer_capture(ctx_tid);
    assert(plan.next == other);
    ctx_other = new_ctx(.id = other, .cat = CAT_BEFORE_AREAD);
    sequencer_resume(ctx_other);
    assert(plan.actions == (ACTION_WAKE | ACTION_YIELD | ACTION_RESUME));
    assert(trace_last(t) == NULL);
    assert(trace_next(t, RECORD_ANY) == NULL);
    assert(expect_processed());
    assert(expect_unmarshaled());
    assert(!expect_marshaled());


    /* replay is over now it should call scheduler_process */
    ctx_other = new_ctx(.id = other, .cat = CAT_BEFORE_AREAD);
    expect_process(ctx_other, false);
    mock.next = tid;

    /* last capture point */
    plan = sequencer_capture(ctx_other);
    assert(plan.next == tid);
    sequencer_resume(ctx_tid);
    // assert(plan.reason == REASON_DET);
    assert(plan.actions == (ACTION_WAKE | ACTION_YIELD | ACTION_RESUME));
    assert(expect_processed());
    assert(!expect_unmarshaled());
    assert(!expect_marshaled());
}

/*******************************************************************************
 * Test record
 ******************************************************************************/

static void
test_record()
{
    PS_PUBLISH_INTERFACE(TOPIC_ENGINE_START, nil);
    printf("Test: %s\n", __FUNCTION__);
    task_id tid    = 1;
    task_id other  = 2;
    record_t *r    = NULL;
    context_t *ctx = NULL;
    plan_t plan    = {0};

    trace_t *t = trace_flat_create(NULL);

    sequencer_reset();
    recorder_init(NULL, t);
    // struct value val = flag_uval(0);
    // PS_PUBLISH_INTERFACE(FLAG_CLI_REPLAY_GOAL, val);

    /* send last event */
    ctx = new_ctx(.id = tid, .cat = CAT_TASK_FINI);
    expect_process(ctx, false);
    mock.next = ANY_TASK;
    r         = record_alloc(0);
    r->id     = other;
    r->clk    = 1;
    r->kind   = RECORD_SCHED;
    plan      = sequencer_capture(ctx);
    assert(plan.next == ANY_TASK);
    // assert(plan.reason == REASON_NDET);
    assert(plan.actions == ACTION_WAKE);
    ctx->id = other;
    sequencer_resume(ctx);
    assert(trace_last(t) != NULL);
    assert(trace_next(t, RECORD_ANY) != NULL);
    assert(memcmp(r, trace_last(t), sizeof(record_t)) != 0);
    assert(expect_processed());
    assert(!expect_unmarshaled());
    assert(expect_marshaled());


    /* send another last event */
    ctx = new_ctx(.id = other, .cat = CAT_TASK_FINI);
    expect_process(ctx, false);
    mock.next = ANY_TASK;
    plan      = sequencer_capture(ctx);
    assert(plan.next == ANY_TASK);
    // assert(plan.reason == REASON_NDET);
    assert(plan.actions == ACTION_WAKE);
    sequencer_resume(ctx);
    assert(trace_last(t) != NULL);
    assert(trace_next(t, RECORD_ANY) != NULL);
    assert(memcmp(r, trace_last(t), sizeof(record_t)) != 0);
    assert(expect_processed());
    assert(expect_marshaled());
    assert(!expect_unmarshaled());
}

// NOLINTEND(bugprone-suspicious-memory-comparison)

int
main(void)
{
    test_main_task();
    test_replay();
    test_record();
    return 0;
}
