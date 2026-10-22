/**
 * @file events.h
 * @brief Task-velocity semantic ingress event identifiers.
 */
#ifndef LOTTO_MODULES_TASK_VELOCITY_EVENTS_H
#define LOTTO_MODULES_TASK_VELOCITY_EVENTS_H

#include <stdint.h>

#ifndef DICE_ROLLED
    #define EVENT_TASK_VELOCITY 154
#endif

typedef struct task_velocity_event {
    int64_t probability;
} task_velocity_event_t;

#endif
