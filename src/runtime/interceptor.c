#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "interceptor.h"
#include <dice/self.h>
#include <lotto/base/category.h>
#include <lotto/base/reason.h>
#include <lotto/base/task_id.h>
#include <lotto/runtime/mediator.h>
#include <lotto/runtime/runtime.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger.h>
#include <lotto/sys/real.h>

#define MAX_FINI 32

typedef void (*fini_t)(void);
static fini_t g_fini[MAX_FINI];
static size_t g_fini_count;

static inline void
handle_status(context_t *ctx, mediator_status_t status)
{
    switch (status) {
        case MEDIATOR_OK:
            return;
        case MEDIATOR_ABORT:
            lotto_exit(ctx, REASON_ABORT);
            break;
        case MEDIATOR_SHUTDOWN:
            lotto_exit(ctx, REASON_SHUTDOWN);
            break;
        default:
            break;
    }
}

static bool
is_task_create(const char *func)
{
    return func != NULL && strcmp(func, "pthread_create") == 0;
}

void
intercept_capture(context_t *ctx)
{
    if (ctx == NULL) {
        return;
    }

    metadata_t *md = ctx->md;
    if (md == NULL) {
        return;
    }
    if (self_retired(md))
        return;

    mediator_t *m = mediator_tls(md);
    if (mediator_capture(m, ctx)) {
        return;
    }

    mediator_status_t status = mediator_resume(m, ctx);
    handle_status(ctx, status);
}

mediator_t *
intercept_before_call(context_t *ctx)
{
    if (ctx == NULL) {
        return NULL;
    }

    metadata_t *md = ctx->md;
    if (md == NULL) {
        return NULL;
    }
    if (self_retired(md))
        return NULL;

    mediator_t *m = mediator_tls(md);
    if (!mediator_capture(m, ctx)) {
        mediator_status_t status = mediator_resume(m, ctx);
        handle_status(ctx, status);
    }
    return m;
}

void
intercept_after_call(context_t *ctx)
{
    if (ctx == NULL) {
        return;
    }

    metadata_t *md = ctx->md;
    if (md == NULL) {
        return;
    }
    if (self_retired(md))
        return;

    if (ctx->func == NULL) {
        ctx->func = "unknown";
    }
    if (ctx->cat == CAT_NONE) {
        ctx->cat = is_task_create(ctx->func) ? CAT_TASK_CREATE : CAT_CALL;
    }

    mediator_t *m = mediator_tls(md);
    mediator_return(m, ctx);
    mediator_status_t status = mediator_resume(m, ctx);
    handle_status(ctx, status);
}

void *
intercept_lookup_call(const char *func)
{
    return real_func(func, 0);
}

void *
intercept_warn_call(const char *func)
{
    log_warnf("warn call '%s'\n", func);
    return real_func(func, 0);
}

void
lotto_intercept_register_fini(fini_t func)
{
    if (g_fini_count < MAX_FINI) {
        g_fini[g_fini_count++] = func;
    }
}

void
lotto_intercept_fini(void)
{
    for (size_t i = 0; i < g_fini_count; ++i) {
        if (g_fini[i] != NULL) {
            g_fini[i]();
        }
    }
    g_fini_count = 0;
}
