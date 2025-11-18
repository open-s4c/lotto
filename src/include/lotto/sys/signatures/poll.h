/*
 */

#ifndef LOTTO_SIGNATURES_POLL_H
#define LOTTO_SIGNATURES_POLL_H

#include <poll.h>
#include <signal.h>

#include <lotto/sys/signatures/defaults_head.h>

#define SYS_POLL                                                               \
    SYS_FUNC(                                                                  \
        LIBC, return,                                                          \
        SIG(int, poll, struct pollfd *, fds, nfds_t, nfds, int, timeout), )
#define SYS_PPOLL                                                              \
    SYS_FUNC(LIBC, return,                                                     \
             SIG(int, ppoll, struct pollfd *, fds, nfds_t, nfds,               \
                 const struct timespec *, timeout_ts, const sigset_t *,        \
                 sigmask), )

#define FOR_EACH_SYS_POLL_WRAPPED                                              \
    SYS_POLL                                                                   \
    SYS_PPOLL

#define FOR_EACH_SYS_POLL_CUSTOM

#define FOR_EACH_SYS_POLL                                                      \
    FOR_EACH_SYS_POLL_WRAPPED                                                  \
    FOR_EACH_SYS_POLL_CUSTOM

#endif // LOTTO_SIGNATURES_POLL_H
