#include "state.h"
#include <lotto/base/flags.h>
#include <lotto/driver/flagmgr.h>
#include <lotto/driver/flags/modules.h>

static void
_busyabort_enabled(bool enabled)
{
    busyabort_config()->enabled = enabled;
}

_FLAGMGR_SUBSCRIBE({ register_runtime_switchable_module(LOTTO_MODULE_NAME,
                                                        _busyabort_enabled); })

NEW_PRETTY_CALLBACK_FLAG(HANDLER_BUSYABORT_ENABLED, "", "handler-busyabort",
                         "enable busy-loop abort handler", flag_on(),
                         STR_CONVERTER_BOOL,
                         { busyabort_config()->enabled = is_on(v); })

NEW_CALLBACK_FLAG(BUSYABORT_THRESHOLD, "", LOTTO_MODULE_FLAG("threshold"), "",
                  "repeat count threshold before abort, 0 for never",
                  flag_uval(~0ULL),
                  { busyabort_config()->threshold = as_uval(v); })
