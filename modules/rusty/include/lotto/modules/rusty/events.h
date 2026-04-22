/**
 * @file events.h
 * @brief Rusty semantic ingress event identifiers.
 */
#ifndef LOTTO_MODULES_RUSTY_EVENTS_H
#define LOTTO_MODULES_RUSTY_EVENTS_H

#include <stdint.h>

#define EVENT_RUSTY_AWAIT      159
#define EVENT_RUSTY_SPIN_START 160
#define EVENT_RUSTY_SPIN_END   161

typedef struct await_event {
    void *addr;
} await_event_t;

typedef struct spin_end_event {
    uint32_t cond;
} spin_end_event_t;

#endif
