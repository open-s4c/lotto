#include "state.h"
#include <lotto/driver/flagmgr.h>
#include <lotto/driver/flags/modules.h>

REGISTER_RUNTIME_SWITCHABLE_CONFIG(race_config(), true)
NEW_CALLBACK_FLAG(HANDLER_RACE_STRICT, "", LOTTO_MODULE_FLAG("strict"), "",
                  "abort when data race detected", flag_off(),
                  { race_config()->abort_on_race = is_on(v); })
