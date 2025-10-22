#include <lotto/base/reason.h>
#include <lotto/base/task_id.h>
#include <lotto/engine/recorder.h>

void
recorder_init(trace_t *input, trace_t *output)
{
    (void)input;
    (void)output;
}

replay_t
recorder_replay(clk_t clk)
{
    (void)clk;
    return (replay_t){
        .status = REPLAY_DONE,
        .id     = NO_TASK,
    };
}

void
recorder_record(const context_t *ctx, clk_t clk)
{
    (void)ctx;
    (void)clk;
}

void
recorder_fini(clk_t clk, task_id id, reason_t reason)
{
    (void)clk;
    (void)id;
    (void)reason;
}

void
recorder_end_trace(void)
{
}

void
recorder_end_replay(void)
{
}
