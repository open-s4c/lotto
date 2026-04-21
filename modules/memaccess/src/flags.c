#include "state.h"
#include <lotto/driver/flagmgr.h>
#include <lotto/driver/flags/modules.h>

static void
_memaccess_enabled(bool enabled)
{
    memaccess_config()->enabled = enabled;
}

_FLAGMGR_SUBSCRIBE({ register_runtime_switchable_module(LOTTO_MODULE_NAME,
                                                        _memaccess_enabled); })

NEW_PRETTY_CALLBACK_FLAG(HANDLER_MEMACCESS_ENABLED, "", "handler-memaccess",
                         "enable memaccess handler", flag_on(),
                         STR_CONVERTER_BOOL,
                         { memaccess_config()->enabled = is_on(v); })
