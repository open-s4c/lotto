/**
 * @file events.h
 * @brief Bias semantic ingress event identifiers.
 */
#ifndef LOTTO_MODULES_BIAS_EVENTS_H
#define LOTTO_MODULES_BIAS_EVENTS_H

#include <lotto/bias.h>

#define EVENT_BIAS_POLICY 190
#define EVENT_BIAS_TOGGLE 191

typedef struct bias_policy_event {
    bias_policy_t policy;
} bias_policy_event_t;

#endif
