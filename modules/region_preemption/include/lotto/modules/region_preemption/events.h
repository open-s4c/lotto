/**
 * @file events.h
 * @brief Region-preemption semantic ingress event identifiers.
 */
#ifndef LOTTO_MODULES_REGION_PREEMPTION_EVENTS_H
#define LOTTO_MODULES_REGION_PREEMPTION_EVENTS_H

#include <stdbool.h>

#define EVENT_REGION_PREEMPTION 155

typedef struct region_preemption_event {
    bool in_region;
} region_preemption_event_t;

#endif
