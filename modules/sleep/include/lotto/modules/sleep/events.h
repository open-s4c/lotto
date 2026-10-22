/**
 * @file events.h
 * @brief Sleep semantic ingress event identifiers.
 */
#ifndef LOTTO_MODULES_SLEEP_EVENTS_H
#define LOTTO_MODULES_SLEEP_EVENTS_H

#include <time.h>

#ifndef DICE_ROLLED
    #define EVENT_SLEEP_YIELD 162
#endif

typedef struct sleep_yield_event {
    const char *func;
    struct timespec duration;
} sleep_yield_event_t;

#endif
