#define LOGGER_BLOCK LOGGER_CUR_BLOCK

#include <lotto/cli/flagmgr.h>
#include <lotto/modules/inactivity_timeout/state.h>

NEW_CALLBACK_FLAG(INACTIVITY_TIMEOUT, "", "inactivity-timeout", "",
                  "set inactivity timeout, 0 for infinity", flag_uval(0),
                  { inactivity_timeout_config()->alarm = as_uval(v); })
