/**
 * @file dispatcher.h
 * @brief Engine declarations for dispatcher.
 */
#ifndef LOTTO_DISPATCHER_H
#define LOTTO_DISPATCHER_H

#include <lotto/base/cappt.h>
#include <lotto/base/context.h>
#include <lotto/base/plan.h>
#include <lotto/check.h>
#include <lotto/engine/pubsub.h>

/**
 * Handles a capture event
 *
 * @param ctx calling context generating event
 * @param e   event object
 */
typedef void (*handle_f)(const context_t *ctx, event_t *e);

/**
 * Dispatches an event to all event handlers
 *
 * @param ctx calling context generating event
 * @param e   event object
 * @return nexttask to be scheduled
 */
task_id dispatch_event(const context_t *ctx, event_t *e);

#define REGISTER_HANDLER(handle)                                               \
    PS_SUBSCRIBE(CHAIN_LOTTO_DEFAULT, EVENT_ENGINE__CAPTURE, {                 \
        const context_t *ctx = (const context_t *)md;                          \
        event_t *e           = (event_t *)event;                               \
        handle(ctx, e);                                                        \
        if (e->skip)                                                           \
            return PS_STOP_CHAIN;                                              \
    })

#define REGISTER_HANDLER_EXTERNAL(handle)                                      \
    PS_SUBSCRIBE(CHAIN_LOTTO_DEFAULT, EVENT_ENGINE__CAPTURE, {                 \
        const context_t *ctx = (const context_t *)md;                          \
        event_t *e           = (event_t *)event;                               \
        if (lotto_loaded())                                                    \
            handle(ctx, e);                                                    \
        if (e->skip)                                                           \
            return PS_STOP_CHAIN;                                              \
    })

#endif
