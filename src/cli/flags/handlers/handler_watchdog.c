/*
 */
#define LOGGER_PREFIX LOGGER_CUR_FILE
#define LOGGER_BLOCK  LOGGER_CUR_BLOCK
#include <stdlib.h>

#include <lotto/cli/flagmgr.h>
#include <lotto/states/handlers/watchdog.h>

#define DEFAULT_BUDGET 100

NEW_PRETTY_CALLBACK_FLAG(HANDLER_WATCHDOG_ENABLED, "", "handler-watchdog",
                         "enable watchdog handler", flag_on(),
                         STR_CONVERTER_BOOL, {
                             watchdog_config()->enabled = is_on(v);
                             const char *var = getenv("LOTTO_WATCHDOG_BUDGET");
                             watchdog_config()->budget =
                                 var != NULL ? strtoull(var, NULL, 10) :
                                               DEFAULT_BUDGET;
                         })
