/**
 * @file context_payload.h
 * @brief Accessors for region-preemption-related context payloads.
 */
#ifndef LOTTO_MODULES_REGION_PREEMPTION_CONTEXT_PAYLOAD_H
#define LOTTO_MODULES_REGION_PREEMPTION_CONTEXT_PAYLOAD_H

#include <stdbool.h>

#include <lotto/modules/region_preemption/events.h>
#include <lotto/runtime/capture_point.h>
#include <lotto/runtime/ingress_events.h>
#include <lotto/sys/assert.h>

static inline bool
capture_point_is_region_preemption_event(const capture_point *cp)
{
    return cp->src_type == EVENT_REGION_PREEMPTION;
}

static inline bool
capture_point_region_preemption_in(const capture_point *cp)
{
    ASSERT(capture_point_is_region_preemption_event(cp));
    ASSERT(cp->payload);
    return ((region_preemption_event_t *)cp->payload)->in_region;
}

#endif
