#include "state.h"
#include <lotto/driver/flagmgr.h>
#include <lotto/driver/flags/modules.h>

static void
_fork_enabled(bool enabled)
{
    fork_execve_config()->enabled = enabled;
}

_FLAGMGR_SUBSCRIBE({ register_runtime_switchable_module(LOTTO_MODULE_NAME,
                                                        _fork_enabled); })

NEW_PRETTY_CALLBACK_FLAG(
    HANDLER_FORK_EXECVE_ENABLED, "", "handler-fork-execve",
    "enable support of fork()+execve(); lotto does not attempt to "
    "control the child process",
    flag_off(), STR_CONVERTER_BOOL,
    { fork_execve_config()->enabled = is_on(v); })
