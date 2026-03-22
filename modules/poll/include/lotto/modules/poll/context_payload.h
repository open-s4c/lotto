/**
 * @file context_payload.h
 * @brief Accessors for poll-related context payloads.
 */
#ifndef LOTTO_MODULES_POLL_CONTEXT_PAYLOAD_H
#define LOTTO_MODULES_POLL_CONTEXT_PAYLOAD_H

#include "events.h"
#include <lotto/runtime/capture_point.h>

typedef enum context_poll_event {
    CONTEXT_POLL_NONE = 0,
    CONTEXT_POLL_WAIT,
} context_poll_event_t;

static inline context_poll_event_t
context_poll_event(const capture_point *cp)
{
    return cp->src_type == EVENT_POLL ? CONTEXT_POLL_WAIT : CONTEXT_POLL_NONE;
}

static inline poll_args_t *
context_poll_args(const capture_point *cp)
{
    ASSERT(cp->src_type == EVENT_POLL);
    return ((poll_event_t *)cp->payload)->args;
}

#endif
