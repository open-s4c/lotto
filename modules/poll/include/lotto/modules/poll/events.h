/**
 * @file events.h
 * @brief Poll semantic ingress event identifiers.
 */
#ifndef LOTTO_MODULES_POLL_EVENTS_H
#define LOTTO_MODULES_POLL_EVENTS_H

#include "poll.h"

#define EVENT_POLL 164

typedef struct poll_event {
    poll_args_t *args;
} poll_event_t;

#endif
