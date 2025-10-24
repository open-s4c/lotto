/*
 */
#define LOG_PREFIX LOG_CUR_FILE
#define LOG_BLOCK  LOG_CUR_BLOCK
#include <lotto/cli/flagmgr.h>
#include <lotto/states/handlers/task_velocity.h>

NEW_PRETTY_CALLBACK_FLAG(HANDLER_TASK_VELOCITY_ENABLED, "",
                         "handler-task-velocity",
                         "enable task velocity handler", flag_on(),
                         STR_CONVERTER_BOOL,
                         { task_velocity_config()->enabled = is_on(v); })
