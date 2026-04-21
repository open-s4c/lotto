#include "state.h"
#include <lotto/driver/flagmgr.h>
#include <lotto/driver/flags/modules.h>

static void
_yield_enabled(bool enabled)
{
    yield_config()->enabled = enabled;
}

_FLAGMGR_SUBSCRIBE({ register_runtime_switchable_module(LOTTO_MODULE_NAME,
                                                        _yield_enabled); })

NEW_CALLBACK_FLAG(ADVISORY_YIELD, "", LOTTO_MODULE_FLAG("advisory"), "",
                  "does not force sched_yield() to result in a context switch",
                  flag_off(), ({ yield_config()->advisory = is_on(v); }))
NEW_PRETTY_CALLBACK_FLAG(HANDLER_YIELD_ENABLED, "", "handler-yield",
                         "enable yield handler", flag_on(), STR_CONVERTER_BOOL,
                         ({ yield_config()->enabled = is_on(v); }))
