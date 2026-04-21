#include <lotto/driver/flagmgr.h>
#include <lotto/driver/flags/modules.h>
#include <lotto/modules/available/state.h>

static void
_available_enabled(bool enabled)
{
    available_config()->enabled = enabled;
}

_FLAGMGR_SUBSCRIBE({ register_runtime_switchable_module(LOTTO_MODULE_NAME,
                                                        _available_enabled); })

NEW_PRETTY_CALLBACK_FLAG(HANDLER_AVAILABLE_ENABLED, "", "handler-available",
                         "enable available handler", flag_on(),
                         STR_CONVERTER_BOOL,
                         { available_config()->enabled = is_on(v); })
