#include "state.h"
#include <lotto/driver/flagmgr.h>

NEW_PRETTY_CALLBACK_FLAG(HANDLER_MEMACCESS_ENABLED, "", "handler-memaccess",
                         "enable memaccess handler", flag_on(),
                         STR_CONVERTER_BOOL,
                         { memaccess_config()->enabled = is_on(v); })
