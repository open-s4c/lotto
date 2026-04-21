#include "state.h"
#include <lotto/driver/flagmgr.h>
#include <lotto/driver/flags/modules.h>

static void
_task_velocity_enabled(bool enabled)
{
    task_velocity_config()->enabled = enabled;
}

_FLAGMGR_SUBSCRIBE({
    register_runtime_switchable_module(LOTTO_MODULE_NAME,
                                       _task_velocity_enabled);
})

NEW_PRETTY_CALLBACK_FLAG(HANDLER_TASK_VELOCITY_ENABLED, "",
                         "handler-task-velocity",
                         "enable task velocity handler", flag_on(),
                         STR_CONVERTER_BOOL,
                         { task_velocity_config()->enabled = is_on(v); })
