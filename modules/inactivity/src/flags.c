#define LOGGER_BLOCK LOGGER_CUR_BLOCK

#include "state.h"
#include <lotto/driver/flagmgr.h>

NEW_CALLBACK_FLAG(INACTIVITY, "", "inactivity", "",
                  "set inactivity timeout, 0 for infinity", flag_uval(0),
                  { inactivity_config()->alarm = as_uval(v); })
