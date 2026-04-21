#include "state.h"
#include <lotto/driver/flags/modules.h>
#include <lotto/driver/flagmgr.h>

static void
_race_enabled(bool enabled)
{
    race_config()->enabled = enabled;
}

_FLAGMGR_SUBSCRIBE({ register_runtime_switchable_module(LOTTO_MODULE_NAME,
                                                        _race_enabled); })

NEW_PRETTY_CALLBACK_FLAG(HANDLER_RACE_ENABLED, "", "handler-race",
                         "enable data race detection handler", flag_on(),
                         STR_CONVERTER_BOOL, {
                             race_config()->enabled = is_on(v);
#ifndef RACE_DEFAULT
                             race_config()->ignore_write_write = true;
                             race_config()->only_write_ichpt   = true;
#endif
                         })
NEW_CALLBACK_FLAG(HANDLER_RACE_STRICT, "", LOTTO_MODULE_FLAG("strict"), "",
                  "abort when data race detected", flag_off(),
                  { race_config()->abort_on_race = is_on(v); })
