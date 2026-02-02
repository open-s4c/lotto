#define LOGGER_PREFIX LOGGER_CUR_FILE
#define LOGGER_BLOCK  LOGGER_CUR_BLOCK
#include <lotto/cli/flagmgr.h>
#include <lotto/modules/atomic/state.h>

NEW_PRETTY_CALLBACK_FLAG(HANDLER_ATOMIC_ENABLED, "", "handler-atomic",
                         "enable atomic handler", flag_on(), STR_CONVERTER_BOOL,
                         { atomic_config()->enabled = is_on(v); })
