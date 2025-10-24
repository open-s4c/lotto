/*
 */
#ifndef _GNU_SOURCE
    #define _GNU_SOURCE
#endif

#include <crep.h>
#include <poll.h>
#include <signal.h>

#include <sys/epoll.h>
#define LOG_PREFIX LOG_CUR_FILE
#define LOG_BLOCK  LOG_CUR_BLOCK
#include <lotto/sys/logger_block.h>

#if 0
CREP_FUNC(SIG(int, poll, struct pollfd *, fds, nfds_t, ndfs, int, timeout),
          MAP(.retsize = sizeof(int),
              .params  = {
                   {
                       .size = ndfs * sizeof(struct pollfd),
                       .data = fds,
                  },
                   NULL,
              }));

CREP_FUNC(SIG(int, ppoll, struct pollfd *, fds, nfds_t, ndfs,
              const struct timespec *, tmo_p, const sigset_t *, sigmask),
          MAP(.retsize = sizeof(int),
              .params  = {
                   {
                       .size = ndfs * sizeof(struct pollfd),
                       .data = fds,
                  },
                   NULL,
              }));
#endif
CREP_FUNC(SIG(int, epoll_wait, int, epfd, struct epoll_event *, events, int,
              maxevents, int, timeout),
          MAP(.retsize = sizeof(int),
              .params  = {
                   {.size = sizeof(struct epoll_event) * maxevents,
                    .data = events},
                   NULL,
              }))
