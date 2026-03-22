/**
 * @file events.h
 * @brief Deadlock semantic ingress event identifiers.
 */
#ifndef LOTTO_MODULES_DEADLOCK_EVENTS_H
#define LOTTO_MODULES_DEADLOCK_EVENTS_H

#define EVENT_RSRC_ACQUIRING     151
#define EVENT_RSRC_RELEASED      152
#define EVENT_DEADLOCK__DETECTED 137

typedef struct rsrc_event {
    void *addr;
} rsrc_event_t;

#endif
