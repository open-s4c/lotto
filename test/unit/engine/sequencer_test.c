#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <dice/events/memaccess.h>
#include <lotto/base/trace_flat.h>
#include <lotto/engine/pubsub.h>
#include <lotto/engine/recorder.h>
#include <lotto/engine/sequencer.h>
#include <lotto/engine/statemgr.h>
#include <lotto/runtime/capture_point.h>
#include <lotto/runtime/ingress_events.h>

// NOLINTBEGIN(bugprone-suspicious-memory-comparison)

#define new_ctx(...)                                                           \
    (&(capture_point){.func     = "UNKNOWN",                                   \
                      .chain_id = CHAIN_INGRESS_EVENT,                         \
                      .type_id  = 0,                                           \
                      .id       = NO_TASK,                                     \
                      .vid      = NO_TASK,                                     \
                      __VA_ARGS__})
void sequencer_reset(void);

/*******************************************************************************
 * mock
 ******************************************************************************/
typedef struct {
    int exit_err;
    task_id wake_id;
    task_id yield_id;
    capture_point *cp;
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

static bool
capture_point_eq(const capture_point *lhs, const capture_point *rhs)
{
    return lhs->chain_id == rhs->chain_id && lhs->type_id == rhs->type_id &&
           lhs->blocking == rhs->blocking && lhs->id == rhs->id &&
           lhs->vid == rhs->vid && lhs->pc == rhs->pc &&
           lhs->cpu_cost == rhs->cpu_cost &&
#if defined(QLOTTO_ENABLED)
           lhs->pstate == rhs->pstate &&
#endif
           lhs->func == rhs->func && lhs->func_addr == rhs->func_addr &&
           lhs->payload == rhs->payload;
}

bool
sequencer_dispatch_override(const capture_point *cp, event_t *e, task_id *next)
{
    assert(mock.expect.cp);
    assert(capture_point_eq(mock.expect.cp, cp));
    assert(mock.expect.skip == e->skip);

    free(mock.expect.cp);
    mock.expect.cp   = NULL;
    mock.expect.skip = false;

    *next = mock.next;
    return true;
}
void
scheduler_postprocess(const capture_point *cp)
{
    assert(mock.expect.cp);
    assert(capture_point_eq(mock.expect.cp, cp));

    free(mock.expect.cp);
    mock.expect.cp = NULL;
}
void
expect_process(const capture_point *cp, bool skip)
{
    if (mock.expect.cp != NULL)
        free(mock.expect.cp);
    mock.expect.cp   = (capture_point *)malloc(sizeof(capture_point));
    *mock.expect.cp  = *cp;
    mock.expect.skip = skip;
}
bool
expect_processed()
{
    return mock.expect.cp == NULL;
}

size_t
statemgr_size(state_type_t type)
{
    return 0;
}

void *
statemgr_marshal(void *buf, state_type_t type)
{
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
statemgr_register(int slot, marshable_t *m, state_type_t type)
{
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
    LOTTO_PUBLISH(EVENT_ENGINE__START, nil);
    printf("Test: %s\n", __FUNCTION__);
    task_id tid = 1;

    capture_point *cp = new_ctx(.id = tid, .type_id = EVENT_TASK_INIT);
    expect_process(cp, false);
    mock.next       = tid;
    mock.now        = 5 * NOW_SECOND;
    struct plan act = sequencer_capture(cp);
    assert(act.next == tid);
    // assert(act.reason == REASON_BLOCKING);
    assert(act.actions == (ACTION_WAKE | ACTION_YIELD | ACTION_RESUME));
}

/*******************************************************************************
 * Test replay
 ******************************************************************************/
static void
test_replay()
{
    LOTTO_PUBLISH(EVENT_ENGINE__START, nil);
    logger(LOGGER_DEBUG, STDERR_FILENO);
    printf("Test: %s\n", __FUNCTION__);
    task_id tid              = 1;
    task_id other            = 2;
    record_t *r              = NULL;
    capture_point *ctx_tid   = NULL;
    capture_point *ctx_other = NULL;
    struct plan plan         = {0};

    trace_t *t = trace_flat_create(NULL);

    sequencer_reset();
    recorder_init(t, NULL);
    // struct value val = flag_uval(3);
    // LOTTO_PUBLISH(FLAG_CLI_REPLAY_GOAL, val);

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
    ctx_tid = new_ctx(.id = tid, .type_id = EVENT_TASK_INIT);
    expect_process(ctx_tid, false);
    plan = sequencer_capture(ctx_tid);
    assert(plan.next == tid);
    sequencer_resume(ctx_tid);
    assert(plan.actions == (ACTION_WAKE | ACTION_YIELD | ACTION_RESUME));
    assert(expect_processed());
    assert(!expect_unmarshaled());
    assert(!expect_marshaled());

    /* capture point 2 */
    ctx_tid = new_ctx(.id = tid, .chain_id = CHAIN_INGRESS_BEFORE,
                      .type_id = EVENT_MA_WRITE, .type_id = EVENT_MA_WRITE);
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
    ctx_tid = new_ctx(.id = tid, .chain_id = CHAIN_INGRESS_BEFORE,
                      .type_id = EVENT_MA_AREAD, .type_id = EVENT_MA_AREAD);
    expect_process(ctx_tid, false);
    plan = sequencer_capture(ctx_tid);
    assert(plan.next == other);
    ctx_other = new_ctx(.id = other, .chain_id = CHAIN_INGRESS_BEFORE,
                        .type_id = EVENT_MA_AREAD, .type_id = EVENT_MA_AREAD);
    sequencer_resume(ctx_other);
    assert(plan.actions == (ACTION_WAKE | ACTION_YIELD | ACTION_RESUME));
    assert(trace_last(t) == NULL);
    assert(trace_next(t, RECORD_ANY) == NULL);
    assert(expect_processed());
    assert(expect_unmarshaled());
    assert(!expect_marshaled());


    /* replay is over now it should call scheduler_process */
    ctx_other = new_ctx(.id = other, .chain_id = CHAIN_INGRESS_BEFORE,
                        .type_id = EVENT_MA_AREAD, .type_id = EVENT_MA_AREAD);
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
    LOTTO_PUBLISH(EVENT_ENGINE__START, nil);
    printf("Test: %s\n", __FUNCTION__);
    task_id tid       = 1;
    task_id other     = 2;
    record_t *r       = NULL;
    capture_point *cp = NULL;
    struct plan plan  = {0};

    trace_t *t = trace_flat_create(NULL);

    sequencer_reset();
    recorder_init(NULL, t);
    // struct value val = flag_uval(0);
    // LOTTO_PUBLISH(FLAG_CLI_REPLAY_GOAL, val);

    /* send last event */
    cp = new_ctx(.id = tid, .type_id = EVENT_TASK_FINI);
    expect_process(cp, false);
    mock.next = ANY_TASK;
    r         = record_alloc(0);
    r->id     = other;
    r->clk    = 1;
    r->kind   = RECORD_SCHED;
    plan      = sequencer_capture(cp);
    assert(plan.next == ANY_TASK);
    // assert(plan.reason == REASON_NDET);
    assert(plan.actions == ACTION_WAKE);
    cp->id = other;
    sequencer_resume(cp);
    assert(trace_last(t) != NULL);
    assert(trace_next(t, RECORD_ANY) != NULL);
    assert(memcmp(r, trace_last(t), sizeof(record_t)) != 0);
    assert(expect_processed());
    assert(!expect_unmarshaled());
    assert(expect_marshaled());


    /* send another last event */
    cp = new_ctx(.id = other, .type_id = EVENT_TASK_FINI);
    expect_process(cp, false);
    mock.next = ANY_TASK;
    plan      = sequencer_capture(cp);
    assert(plan.next == ANY_TASK);
    // assert(plan.reason == REASON_NDET);
    assert(plan.actions == ACTION_WAKE);
    sequencer_resume(cp);
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
