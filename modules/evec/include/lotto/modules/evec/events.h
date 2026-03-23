/**
 * @file events.h
 * @brief Evec semantic ingress event identifiers.
 */
#ifndef LOTTO_MODULES_EVEC_EVENTS_H
#define LOTTO_MODULES_EVEC_EVENTS_H

#include <stdint.h>
#include <time.h>

#include <lotto/evec.h>

#define EVENT_EVEC_PREPARE    145
#define EVENT_EVEC_WAIT       146
#define EVENT_EVEC_TIMED_WAIT 147
#define EVENT_EVEC_CANCEL     148
#define EVENT_EVEC_WAKE       149
#define EVENT_EVEC_MOVE       150

struct evec_prepare_event {
    const void *pc;
    void *addr;
};

struct evec_wait_event {
    const void *pc;
    void *addr;
};

struct evec_timed_wait_event {
    const void *pc;
    void *addr;
    const struct timespec *abstime;
    enum lotto_timed_wait_status ret;
};

struct evec_cancel_event {
    const void *pc;
    void *addr;
};

struct evec_wake_event {
    const void *pc;
    void *addr;
    uint32_t cnt;
};

struct evec_move_event {
    const void *pc;
    void *src;
    void *dst;
};

#endif
