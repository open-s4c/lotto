/**
 * @file events.h
 * @brief Priority semantic ingress event identifiers.
 */
#ifndef LOTTO_MODULES_PRIORITY_EVENTS_H
#define LOTTO_MODULES_PRIORITY_EVENTS_H

#include <stdint.h>

#define EVENT_PRIORITY 153

typedef struct priority_event {
    int64_t priority;
} priority_event_t;

#endif
