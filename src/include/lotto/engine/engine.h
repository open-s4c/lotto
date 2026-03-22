/**
 * @file engine.h
 * @brief Internal engine declarations for engine.
 */
#ifndef LOTTO_ENGINE_H
#define LOTTO_ENGINE_H

#include <lotto/runtime/capture_point.h>
#include <lotto/engine/plan.h>
#include <lotto/base/reason.h>
#include <lotto/base/trace.h>

/**
 * Initializes engine with input-trace configuration.
 */
void engine_init(trace_t *input, trace_t *output);

/**
 * Terminates sequencer and returns _exit status code.
 *
 * engine_fini can be called from signal unsafe contexts, eg, from signal
 * handlers.
 */
int engine_fini(const capture_point *cp, reason_t reason);

/**
 * Returns an action for a given captured context.
 *
 * The action has to be fulfilled following the expected contract.
 */
struct plan engine_capture(const capture_point *cp);

/**
 * Informs engine that task is resuming after an ACTION_YIELD.
 */
void engine_resume(const capture_point *cp);

/**
 * Informs engine that the task has returned from a call.
 */
void engine_return(const capture_point *cp);

#endif
