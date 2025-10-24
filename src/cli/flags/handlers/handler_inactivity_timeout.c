/*
 */
#define LOG_PREFIX LOG_CUR_FILE
#define LOG_BLOCK  LOG_CUR_BLOCK

#include <lotto/cli/flagmgr.h>
#include <lotto/states/handlers/inactivity_timeout.h>

NEW_CALLBACK_FLAG(INACTIVITY_TIMEOUT, "", "inactivity-timeout", "",
                  "set inactivity timeout, 0 for infinity", flag_uval(0),
                  { inactivity_timeout_config()->alarm = as_uval(v); })
