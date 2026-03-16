/**
 * @file poll.h
 * @brief Poll module declarations for poll.
 */
#ifndef LOTTO_MODULES_POLL_POLL_H
#define LOTTO_MODULES_POLL_POLL_H

#ifndef _GNU_SOURCE
    #define _GNU_SOURCE
#endif
#include <poll.h>

typedef struct poll_args_s {
    struct pollfd *fds;
    nfds_t nfds;
    int timeout;
    int ret;
} poll_args_t;

#endif
