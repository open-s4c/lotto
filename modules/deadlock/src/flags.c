#include "state.h"
#include <lotto/driver/flagmgr.h>
#include <lotto/driver/flags/modules.h>

static void
_deadlock_enabled(bool enabled)
{
    deadlock_config()->enabled = enabled;
}

_FLAGMGR_SUBSCRIBE({ register_runtime_switchable_module(LOTTO_MODULE_NAME,
                                                        _deadlock_enabled); })

NEW_PRETTY_CALLBACK_FLAG(HANDLER_DEADLOCK_ENABLED, "", "handler-deadlock",
                         "enable deadlock handler", flag_on(),
                         STR_CONVERTER_BOOL,
                         { deadlock_config()->enabled = is_on(v); })

NEW_CALLBACK_FLAG(HANDLER_DEADLOCK_LOST_RESOURCE_CHECK, "",
                  LOTTO_MODULE_FLAG("check-lost-resource"), "",
                  "enable deadlock handler's lost resource detection",
                  flag_on(),
                  { deadlock_config()->lost_resource_check = is_on(v); })

NEW_CALLBACK_FLAG(HANDLER_DEADLOCK_EXTRA_RELEASE_CHECK, "",
                  LOTTO_MODULE_FLAG("check-extra-release"), "",
                  "enable deadlock handler's extra release detection",
                  flag_on(),
                  { deadlock_config()->extra_release_check = is_on(v); })
