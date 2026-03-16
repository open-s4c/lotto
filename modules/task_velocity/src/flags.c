#define LOGGER_BLOCK LOGGER_CUR_BLOCK
#include <lotto/driver/flagmgr.h>
#include "state.h"

NEW_PRETTY_CALLBACK_FLAG(HANDLER_TASK_VELOCITY_ENABLED, "",
                         "handler-task-velocity",
                         "enable task velocity handler", flag_on(),
                         STR_CONVERTER_BOOL,
                         { task_velocity_config()->enabled = is_on(v); })
