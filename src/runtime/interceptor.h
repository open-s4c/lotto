#ifndef LOTTO_RUNTIME_INTERCEPTOR_H
#define LOTTO_RUNTIME_INTERCEPTOR_H

#include <stdbool.h>

#define EVENT_SCHED_YIELD 103

#include <dice/types.h>
#include <lotto/base/context.h>

/* Forward declaration to avoid pulling mediator internals for the stubs. */
typedef struct mediator mediator_t;

mediator_t *intercept_before_call(context_t *ctx);
void intercept_after_call(context_t *ctx);
void intercept_capture(context_t *ctx);
void lotto_intercept_register_fini(void (*func)(void)) __attribute__((weak));
void lotto_intercept_fini(void) __attribute__((weak));
void *intercept_lookup_call(const char *func) __attribute__((weak));
void *intercept_warn_call(const char *func) __attribute__((weak));

#endif /* LOTTO_RUNTIME_INTERCEPTOR_H */
