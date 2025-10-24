/*
 */
#ifndef LOTTO_SEQUENCER_H
#define LOTTO_SEQUENCER_H

#include <lotto/base/context.h>
#include <lotto/base/plan.h>
#include <lotto/base/reason.h>

/**
 * Terminates sequencer
 */
void sequencer_fini(const context_t *ctx, reason_t reason);

/**
 * Returns an action for a given captured context.
 *
 * The action has to be fulfilled following the expected contract.
 */
plan_t sequencer_capture(const context_t *ctx);

/**
 * Informs sequencer that task is resuming after an ACTION_YIELD.
 */
void sequencer_resume(const context_t *ctx);

/**
 * Informs the sequencer that the task has returned from a call.
 */
void sequencer_return(const context_t *ctx);

/**
 * Returns the current clock
 */
clk_t sequencer_get_clk();

#endif
