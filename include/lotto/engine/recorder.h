#ifndef LOTTO_ENGINE_RECORDER_H
#define LOTTO_ENGINE_RECORDER_H

#include <stdbool.h>

#include <lotto/base/clk.h>
#include <lotto/base/context.h>
#include <lotto/base/task_id.h>
#include <lotto/base/trace.h>
#include <lotto/util/macros.h>

typedef struct replay {
    enum replay_status {
        REPLAY_DONE,  //< replay is over or never started
        REPLAY_LOAD,  //< replay is on and new state was loaded
        REPLAY_FORCE, //< replay is on and new externally modified state
                      // was loaded
        REPLAY_CONT,  //< replay is on and caller should continue
        REPLAY_NEXT,  //< replay is on and id decision was overruled (ANY_TASK)
        REPLAY_LAST,  //< replay is over just now, this was the last record
    } status;
    task_id id;
} replay_t;

/**
 * Record the current state at clock `clk`.
 * TODO: maybe reason needs to be passed.
 *
 * @param ctx the capture point context
 * @param clk the current clock being resumed
 */
void recorder_record(const context_t *ctx, clk_t clk);

/**
 * Replay by unmarshaling the current record if `clk` matches.
 *
 * @param clk the current clock about to start
 * @return a structure describing the replay status
 */
replay_t recorder_replay(clk_t clk);

/**
 * Records a final record at the given clock.
 *
 * The final record is preallocated and it is safe to call this function from
 * signal handlers since it makes no allocations.
 *
 * The no further recorder function shall be called again after recorder_fini.
 *
 * @param clk current clock
 * @param id the task finalizing the recorder
 * @param reason an informative reason
 */
void recorder_fini(clk_t clk, task_id id, reason_t reason);

/**
 * Initialize the recorder with input and output traces.
 *
 * @param input trace to be replayed, may be NULL
 * @param output trace to record, may be NULL
 */

void recorder_init(trace_t *input, trace_t *output);

/**
 * Mark the end of the load of the replay trace.
 *
 * This function does not do anything, but can be used to set break points and
 * learn when the replay is over.
 */
void recorder_end_trace();

/**
 * Mark the end of the replay.
 *
 * This function does not do anything, but can be used to set break points and
 * learn when the replay is over.
 */
void recorder_end_replay();


#endif
