/*
 */
#define LOG_PREFIX LOG_CUR_FILE
#define LOG_BLOCK  LOG_CUR_BLOCK
#include <state.h>

#include <lotto/cli/flagmgr.h>

NEW_PRETTY_CALLBACK_FLAG(HANDLER_PRIORITY_ENABLED, "", "handler-user-priority",
                         "enable user priority handler", flag_on(),
                         STR_CONVERTER_BOOL,
                         { priority_config()->enabled = is_on(v); })
