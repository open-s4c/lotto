#define LOGGER_PREFIX LOGGER_CUR_FILE
#define LOGGER_BLOCK  LOGGER_CUR_BLOCK
#include <lotto/cli/flagmgr.h>
#include <lotto/modules/yield/state.h>

NEW_CALLBACK_FLAG(ADVISORY_YIELD, "", "advisory-yield", "",
                  "does not force sched_yield() to result in a context switch",
                  flag_off(), ({ yield_config()->advisory = is_on(v); }))
NEW_PRETTY_CALLBACK_FLAG(HANDLER_YIELD_ENABLED, "", "handler-yield",
                         "enable yield handler", flag_on(), STR_CONVERTER_BOOL,
                         ({ yield_config()->enabled = is_on(v); }))
