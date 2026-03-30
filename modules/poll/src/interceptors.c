#include <alloca.h>
#include <poll.h>
#include <signal.h>
#include <time.h>

#include "poll.h"
#include <dice/chains/intercept.h>
#include <dice/module.h>
#include <dice/pubsub.h>
#include <lotto/base/callrec.h>
#include <lotto/base/record.h>
#include <lotto/base/trace.h>
#include <lotto/engine/clock.h>
#include <lotto/engine/prng.h>
#include <lotto/engine/pubsub.h>
#include <lotto/engine/recorder.h>
#include <lotto/modules/poll/events.h>
#include <lotto/sys/logger.h>
#include <lotto/sys/real.h>
#include <lotto/sys/sched.h>
#include <lotto/unsafe/time.h>
#include <lotto/util/casts.h>
#include <sys/types.h>

static void
intercept_poll(poll_args_t *args)
{
    poll_event_t ev = {.args = args};
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_POLL, &ev, 0);
}

int
poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
    poll_args_t args = {.fds = fds, .nfds = nfds, .timeout = timeout};
    intercept_poll(&args);
    return args.ret;
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
