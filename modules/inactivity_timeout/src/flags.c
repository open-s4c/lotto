#define LOGGER_BLOCK LOGGER_CUR_BLOCK

#include <lotto/driver/flagmgr.h>
#include "state.h"

NEW_CALLBACK_FLAG(INACTIVITY_TIMEOUT, "", "inactivity-timeout", "",
                  "set inactivity timeout, 0 for infinity", flag_uval(0),
                  { inactivity_timeout_config()->alarm = as_uval(v); })
