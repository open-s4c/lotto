#define LOGGER_PREFIX LOGGER_CUR_FILE
#define LOGGER_BLOCK  LOGGER_CUR_BLOCK
#include <lotto/cli/flagmgr.h>
#include <lotto/states/handlers/available.h>

NEW_PRETTY_CALLBACK_FLAG(HANDLER_AVAILABLE_ENABLED, "", "handler-available",
                         "enable available handler", flag_on(),
                         STR_CONVERTER_BOOL,
                         { available_config()->enabled = is_on(v); })
