/*
 */
#ifndef LOTTO_ENGINE_H
#define LOTTO_ENGINE_H

#include <lotto/base/context.h>
#include <lotto/base/plan.h>
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
int engine_fini(const context_t *ctx, reason_t reason);

/**
 * Returns an action for a given captured context.
 *
 * The action has to be fulfilled following the expected contract.
 */
plan_t engine_capture(const context_t *ctx);

/**
 * Informs engine that task is resuming after an ACTION_YIELD.
 */
void engine_resume(const context_t *ctx);

/**
 * Informs engine that the task has returned from a call.
 */
void engine_return(const context_t *ctx);

#endif
