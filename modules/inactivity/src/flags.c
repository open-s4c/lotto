
#include "state.h"
#include <lotto/driver/flagmgr.h>

NEW_CALLBACK_FLAG(INACTIVITY, "", LOTTO_MODULE_FLAG("timeout"), "",
                  "set inactivity timeout, 0 for infinity", flag_uval(0),
                  { inactivity_config()->alarm = as_uval(v); })
