#include "state.h"
#include <lotto/driver/flagmgr.h>
#include <lotto/driver/flags/modules.h>

static void
_priority_enabled(bool enabled)
{
    priority_config()->enabled = enabled;
}

_FLAGMGR_SUBSCRIBE({ register_runtime_switchable_module(LOTTO_MODULE_NAME,
                                                        _priority_enabled); })

NEW_PRETTY_CALLBACK_FLAG(HANDLER_PRIORITY_ENABLED, "", "handler-user-priority",
                         "enable user priority handler", flag_on(),
                         STR_CONVERTER_BOOL,
                         { priority_config()->enabled = is_on(v); })
