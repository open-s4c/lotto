/*
 */
#define LOGGER_PREFIX LOGGER_CUR_FILE
#define LOGGER_BLOCK  LOGGER_CUR_BLOCK
#include <state.h>

#include <lotto/cli/flagmgr.h>

NEW_PRETTY_CALLBACK_FLAG(
    HANDLER_FORK_EXECVE_ENABLED, "", "handler-fork-execve",
    "enable support of fork()+execve(); lotto does not attempt to "
    "control the child process",
    flag_off(), STR_CONVERTER_BOOL,
    { fork_execve_config()->enabled = is_on(v); })
