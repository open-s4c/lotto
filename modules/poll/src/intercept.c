#include <alloca.h>
#include <poll.h>
#include <signal.h>
#include <time.h>

#define LOGGER_BLOCK LOGGER_CUR_BLOCK
#include <lotto/base/callrec.h>
#include <lotto/base/record.h>
#include <lotto/base/topic.h>
#include <lotto/base/trace.h>
#include <lotto/brokers/pubsub.h>
#include <lotto/cli/trace_utils.h>
#include <lotto/engine/clock.h>
#include <lotto/engine/prng.h>
#include <lotto/engine/recorder.h>
#include <lotto/modules/poll.h>
#include <lotto/runtime/intercept.h>
#include <lotto/sys/logger_block.h>
#include <lotto/sys/real.h>
#include <lotto/sys/sched.h>
#include <lotto/unsafe/time.h>
#include <lotto/util/casts.h>
#include <sys/types.h>

int
poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
    poll_args_t args = {.fds = fds, .nfds = nfds, .timeout = timeout};
    intercept_capture(
        ctx(.func = __FUNCTION__, .cat = CAT_POLL, .args = {arg_ptr(&args)}));
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
