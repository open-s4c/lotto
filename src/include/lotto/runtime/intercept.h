/*
 */
#ifndef LOTTO_INTERCEPT_H
#define LOTTO_INTERCEPT_H

#include <dice/self.h>
#include <lotto/base/context.h>
#include <lotto/base/task_id.h>
#include <lotto/runtime/mediator.h>
#include <lotto/util/macros.h>

/*******************************************************************************
 * mediator's task_id (in TLS)
 ******************************************************************************/

static inline task_id
get_task_id(void)
{
    mediator_t *m = mediator_tls(self_md());
    ASSERT((!!m) && "thread-specific mediator data not initialized");
    return (m->id);
}

/*
 * Return the mediator of the current task, init flag also initializes
 * mediator if not yet.
 */
mediator_t *mediator_tls(struct metadata *md);

/*******************************************************************************
 * guarded lotto_step
 ******************************************************************************/

void intercept_capture(context_t *ctx);

/* called before executing an asm-intercepted external function. */
void *intercept_lookup_call(const char *func);

/* called before executing an intercepted function with known context.
 */
mediator_t *intercept_before_call(context_t *ctx);

/* called after executing an intercepted external function. */
void intercept_after_call(const char *func);

/// true if lotto interceptors have been initialized
bool lotto_intercept_initialized(void);

void *intercept_warn_call(const char *func);

#define MAX_FINI 100
typedef void (*fini_t)();
void lotto_intercept_register_fini(fini_t func);
void lotto_intercept_fini();

#endif
