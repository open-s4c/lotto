#ifndef LOTTO_STATE_POLL_H
#define LOTTO_STATE_POLL_H

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
