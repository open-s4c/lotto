#ifndef LOTTO_ENGINE_ENGINE_H
#define LOTTO_ENGINE_ENGINE_H

#include <lotto/base/context.h>
#include <lotto/base/plan.h>
#include <lotto/base/reason.h>
#include <lotto/base/trace.h>

void engine_init(trace_t *input, trace_t *output);
int engine_fini(const context_t *ctx, reason_t reason);
plan_t engine_capture(const context_t *ctx);
void engine_resume(const context_t *ctx);
void engine_return(const context_t *ctx);

#endif /* LOTTO_ENGINE_ENGINE_H */
