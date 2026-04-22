/**
 * @file ingress_events.h
 * @brief Lotto ingress semantic event identifiers.
 */
#ifndef LOTTO_RUNTIME_INGRESS_EVENTS_H
#define LOTTO_RUNTIME_INGRESS_EVENTS_H

#include <stdint.h>

#define EVENT_TASK_INIT    170
#define EVENT_TASK_FINI    171
#define EVENT_TASK_CREATE  172
#define EVENT_TASK_DETACH  174
#define EVENT_TASK_JOIN    176

typedef struct task_join_event {
    uintptr_t thread;
    void **ptr;
    int ret;
} task_join_event_t;

#endif
