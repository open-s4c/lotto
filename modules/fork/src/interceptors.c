
#include "state.h"
#include <dice/chains/intercept.h>
#include <lotto/engine/pubsub.h>
#include <lotto/modules/fork/events.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger.h>
#include <lotto/sys/unistd.h>

typedef struct {
    const char *func;
} fork_event_t;

pid_t
lotto_fork_execve(const char *pathname, char *const argv[], char *const envp[])
{
    if (!fork_execve_config()->enabled) {
        return -1;
    }

    pid_t ret       = -1;
    fork_event_t ev = {.func = __FUNCTION__};

    PS_PUBLISH(INTERCEPT_BEFORE, EVENT_FORK_EXECVE, &ev, 0);

    ret = sys_fork();

    if (ret != 0) {
        PS_PUBLISH(INTERCEPT_AFTER, EVENT_FORK_EXECVE, &ev, 0);
    } else {
        ASSERT(sys_execve(pathname, argv, envp) == 0 &&
               "Error while calling execve");
        __builtin_unreachable();
    }

    return ret;
}
