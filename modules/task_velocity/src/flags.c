#include "state.h"
#include <lotto/driver/flagmgr.h>

NEW_PRETTY_CALLBACK_FLAG(HANDLER_TASK_VELOCITY_ENABLED, "",
                         "handler-task-velocity",
                         "enable task velocity handler", flag_on(),
                         STR_CONVERTER_BOOL,
                         { task_velocity_config()->enabled = is_on(v); })
