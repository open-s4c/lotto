#ifndef LOTTO_DISPATCHER_H
#define LOTTO_DISPATCHER_H

#include <lotto/base/cappt.h>
#include <lotto/base/context.h>
#include <lotto/base/plan.h>
#include <lotto/base/slot.h>
#include <lotto/check.h>

/**
 * Handles a capture event
 *
 * @param ctx calling context generating event
 * @param e   event object
 */
typedef void (*handle_f)(const context_t *ctx, event_t *e);

/**
 * Register event handler in dispatcher
 *
 * @param slot unique slot number
 * @param handle event handler callback
 *
 * Slots are processes in increasing order.
 */
void dispatcher_register(slot_t slot, handle_f handle);

/**
 * Dispatches an event to all event handlers
 *
 * @param ctx calling context generating event
 * @param e   event object
 * @return nexttask to be scheduled
 */
task_id dispatch_event(const context_t *ctx, event_t *e);

#define REGISTER_HANDLER(slot, handle)                                         \
    static void LOTTO_CONSTRUCTOR _dispatcher_register_##slot(void)            \
    {                                                                          \
        dispatcher_register(slot, handle);                                     \
    }

#define REGISTER_HANDLER_EXTERNAL(slot, handle)                                \
    static void LOTTO_CONSTRUCTOR _dispatcher_register_##slot(void)            \
    {                                                                          \
        if (lotto_loaded())                                                    \
            dispatcher_register(slot, handle);                                 \
    }

#endif
