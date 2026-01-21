/*
 */
/**
 */
#define LOGGER_PREFIX LOGGER_CUR_FILE
#define LOGGER_BLOCK  LOGGER_CUR_BLOCK

#include <state.h>

#include <lotto/base/tidmap.h>
#include <lotto/brokers/statemgr.h>
#include <lotto/engine/dispatcher.h>
#include <lotto/runtime/intercept.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger_block.h>
#include <lotto/sys/unistd.h>
#include <lotto/util/macros.h>
#include <lotto/util/once.h>

pid_t
lotto_fork_execve(const char *pathname, char *const argv[], char *const envp[])
{
    if (!fork_execve_config()->enabled) {
        return -1;
    }

    pid_t ret = -1;

    intercept_before_call(ctx(.func = __FUNCTION__, .cat = CAT_CALL));

    ret = sys_fork();

    if (ret != 0) {
        intercept_after_call(__FUNCTION__);
    } else {
        ASSERT(sys_execve(pathname, argv, envp) == 0 &&
               "Error while calling execve");
        __builtin_unreachable();
    }

    return ret;
}
