#include "state.h"
#include <lotto/driver/flagmgr.h>
#include <lotto/driver/flags/modules.h>

static void
_region_filter_enabled(bool enabled)
{
    region_filter_config()->enabled = enabled;
}

_FLAGMGR_SUBSCRIBE({ register_runtime_switchable_module(LOTTO_MODULE_NAME,
                                                        _region_filter_enabled); })

NEW_PRETTY_CALLBACK_FLAG(REGION_FILTER, "", "handler-region-filter",
                         "enable region filter handler",
#ifdef QLOTTO_ENABLED
                         flag_on(),
#else
            flag_off(),
#endif
                         STR_CONVERTER_BOOL,
                         { region_filter_config()->enabled = is_on(v); })
