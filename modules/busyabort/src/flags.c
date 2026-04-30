#include "state.h"
#include <lotto/base/flags.h>
#include <lotto/driver/flagmgr.h>
#include <lotto/driver/flags/modules.h>

REGISTER_RUNTIME_SWITCHABLE_CONFIG(busyabort_config(), false)

NEW_CALLBACK_FLAG(BUSYABORT_THRESHOLD, "", LOTTO_MODULE_FLAG("threshold"), "",
                  "repeat count threshold before abort, 0 for never",
                  flag_uval(1000000),
                  { busyabort_config()->threshold = as_uval(v); })
