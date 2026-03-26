#include <lotto/driver/flagmgr.h>
#include <lotto/modules/available/state.h>

NEW_PRETTY_CALLBACK_FLAG(HANDLER_AVAILABLE_ENABLED, "", "handler-available",
                         "enable available handler", flag_on(),
                         STR_CONVERTER_BOOL,
                         { available_config()->enabled = is_on(v); })
