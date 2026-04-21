#include "state.h"
#include <lotto/driver/flagmgr.h>
#include <lotto/driver/flags/modules.h>

static void
_region_preemption_enabled(bool enabled)
{
    region_preemption_config()->enabled = enabled;
}

_FLAGMGR_SUBSCRIBE({
    register_runtime_switchable_module(LOTTO_MODULE_NAME,
                                       _region_preemption_enabled);
})

NEW_PRETTY_CALLBACK_FLAG(
    HANDLER_REGION_PREEMPTION_ENABLED, "", "region-preemption",
    "avoid context switches unless inside a preemption region", flag_on(),
    STR_CONVERTER_BOOL, { region_preemption_config()->enabled = is_on(v); })
NEW_CALLBACK_FLAG(REGION_PREEMPTION_DEFAULT_OFF, "",
                  LOTTO_MODULE_FLAG("default-off"), "",
                  "disables preemptions", flag_off(),
                  { region_preemption_config()->default_on = is_off(v); })
