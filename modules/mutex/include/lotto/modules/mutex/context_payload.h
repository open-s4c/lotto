/**
 * @file context_payload.h
 * @brief Accessors for mutex-related context payloads.
 */
#ifndef LOTTO_MODULES_MUTEX_CONTEXT_PAYLOAD_H
#define LOTTO_MODULES_MUTEX_CONTEXT_PAYLOAD_H

#include <stdint.h>

#include <lotto/base/context.h>
#include <lotto/modules/mutex/events.h>
#include <lotto/runtime/capture_point.h>
#include <lotto/runtime/context_payload.h>

typedef enum context_mutex_event {
    CONTEXT_MUTEX_NONE = 0,
    CONTEXT_MUTEX_ACQUIRE,
    CONTEXT_MUTEX_TRYACQUIRE,
    CONTEXT_MUTEX_RELEASE,
} context_mutex_event_t;

static inline context_mutex_event_t
context_mutex_event(const context_t *ctx)
{
    switch (context_event_type(ctx)) {
        case EVENT_MUTEX_ACQUIRE:
            return CONTEXT_MUTEX_ACQUIRE;
        case EVENT_MUTEX_TRYACQUIRE:
            return CONTEXT_MUTEX_TRYACQUIRE;
        case EVENT_MUTEX_RELEASE:
            return CONTEXT_MUTEX_RELEASE;
        default:
            return CONTEXT_MUTEX_NONE;
    }
}

static inline uint64_t
context_mutex_addr(const context_t *ctx)
{
    ASSERT(context_has_capture_point(ctx));
    switch (context_mutex_event(ctx)) {
        case CONTEXT_MUTEX_ACQUIRE:
            return (uint64_t)(uintptr_t)((struct mutex_acquire_event *)
                                             ctx->cp->payload)
                ->addr;
        case CONTEXT_MUTEX_TRYACQUIRE:
            return (uint64_t)(uintptr_t)((struct mutex_tryacquire_event *)
                                             ctx->cp->payload)
                ->addr;
        case CONTEXT_MUTEX_RELEASE:
            return (uint64_t)(uintptr_t)((struct mutex_release_event *)
                                             ctx->cp->payload)
                ->addr;
        default:
            ASSERT(0);
            return 0;
    }
}

static inline bool
context_mutex_try_ok(const context_t *ctx)
{
    ASSERT(context_mutex_event(ctx) == CONTEXT_MUTEX_TRYACQUIRE);
    ASSERT(context_has_capture_point(ctx));
    return ((struct mutex_tryacquire_event *)ctx->cp->payload)->ret == 0;
}

static inline void
context_mutex_try_set_ret(const context_t *ctx, int ret)
{
    ASSERT(context_mutex_event(ctx) == CONTEXT_MUTEX_TRYACQUIRE);
    ASSERT(context_has_capture_point(ctx));
    ((struct mutex_tryacquire_event *)ctx->cp->payload)->ret = ret;
}

#endif
