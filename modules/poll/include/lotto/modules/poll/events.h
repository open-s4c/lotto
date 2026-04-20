/**
 * @file events.h
 * @brief Poll semantic ingress event identifiers.
 */
#ifndef LOTTO_MODULES_POLL_EVENTS_H
#define LOTTO_MODULES_POLL_EVENTS_H

#define EVENT_POLL 164

#include <poll.h>

struct poll_event {
    const void *pc;
    struct pollfd *fds;
    nfds_t nfds;
    int timeout;
    int ret;
    int (*func)(struct pollfd *, nfds_t, int);
};

#endif
