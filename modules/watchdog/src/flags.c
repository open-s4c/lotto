#include <stdlib.h>

#include "state.h"
#include <lotto/driver/flagmgr.h>
#include <lotto/driver/flags/modules.h>

#define DEFAULT_BUDGET 100

static void
_watchdog_enabled(bool enabled)
{
    watchdog_config()->enabled = enabled;
}

_FLAGMGR_SUBSCRIBE({ register_runtime_switchable_module(LOTTO_MODULE_NAME,
                                                        _watchdog_enabled); })

NEW_PRETTY_CALLBACK_FLAG(HANDLER_WATCHDOG_ENABLED, "", "handler-watchdog",
                         "enable watchdog handler", flag_on(),
                         STR_CONVERTER_BOOL, {
                             watchdog_config()->enabled = is_on(v);
                             const char *var = getenv("LOTTO_WATCHDOG_BUDGET");
                             watchdog_config()->budget =
                                 var != NULL ? strtoull(var, NULL, 10) :
                                               DEFAULT_BUDGET;
                         })
