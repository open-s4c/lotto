#ifndef LOTTO_RUNTIME_INTERCEPT_H
#define LOTTO_RUNTIME_INTERCEPT_H

#include <stdbool.h>

#include <lotto/base/context.h>

/* Forward declaration to avoid pulling mediator internals for the stubs. */
typedef struct mediator mediator_t;

/*
 * The real runtime provides strong definitions for these hooks.  In the staged
 * build we only have weak stubs so callers can safely invoke them even when
 * the heavy runtime pieces have not been ported yet.
 */
mediator_t *intercept_before_call(context_t *ctx) __attribute__((weak));
void intercept_after_call(const char *func) __attribute__((weak));
void intercept_capture(context_t *ctx) __attribute__((weak));
bool lotto_intercept_initialized(void) __attribute__((weak));

#endif /* LOTTO_RUNTIME_INTERCEPT_H */

