#include "state.h"
#include <lotto/driver/flagmgr.h>
#include <lotto/driver/flags/modules.h>

REGISTER_RUNTIME_SWITCHABLE_CONFIG(yield_config(), true)

NEW_CALLBACK_FLAG(ADVISORY_YIELD, "", LOTTO_MODULE_FLAG("advisory"), "",
                  "does not force sched_yield() to result in a context switch",
                  flag_off(), ({ yield_config()->advisory = is_on(v); }))
