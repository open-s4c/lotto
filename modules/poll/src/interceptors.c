#include <poll.h>
#include <signal.h>
#include <time.h>

#include "poll.h"
#include <dice/chains/intercept.h>
#include <dice/interpose.h>
#include <dice/module.h>
#include <dice/pubsub.h>
#include <lotto/base/record.h>
#include <lotto/base/trace.h>
#include <lotto/engine/clock.h>
#include <lotto/engine/prng.h>
#include <lotto/engine/pubsub.h>
#include <lotto/engine/recorder.h>
#include <lotto/modules/poll/events.h>
#include <lotto/sys/logger.h>
#include <lotto/unsafe/time.h>

INTERPOSE(int, poll, struct pollfd *fds, nfds_t nfds, int timeout)
{
    struct poll_event ev = {
        .pc      = INTERPOSE_PC,
        .fds     = fds,
        .nfds    = nfds,
        .timeout = timeout,
        .ret     = 0,
        .func    = REAL_FUNC(poll),
    };

    struct metadata md = {0};
    PS_PUBLISH(INTERCEPT_BEFORE, EVENT_POLL, &ev, &md);
    int ret = ev.func(ev.fds, ev.nfds, ev.timeout);
    if (ev.ret == 0)
        ev.ret = ret;
    PS_PUBLISH(INTERCEPT_AFTER, EVENT_POLL, &ev, &md);
    return ev.ret;
}

int
ppoll(struct pollfd *fds, nfds_t nfds, const struct timespec *timeout_ts,
      const sigset_t *sigmask)
{
    sigset_t origmask;
    pthread_sigmask(SIG_SETMASK, sigmask, &origmask);
    int timeout = timeout_ts == NULL ?
                      -1 :
                      (int)(timeout_ts->tv_sec * MSEC_IN_SEC +
                            timeout_ts->tv_nsec / NSEC_IN_MSEC);
    if (timeout < -1 || timeout > 100) {
        timeout = 100;
    }
    int result = poll(fds, nfds, timeout);
    pthread_sigmask(SIG_SETMASK, &origmask, NULL);
    return result;
}
