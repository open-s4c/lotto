/*
 */
#define LOG_PREFIX LOG_CUR_FILE
#define LOG_BLOCK  LOG_CUR_BLOCK
#include <lotto/cli/flagmgr.h>
#include <lotto/states/handlers/atomic.h>

NEW_PRETTY_CALLBACK_FLAG(HANDLER_ATOMIC_ENABLED, "", "handler-atomic",
                         "enable atomic handler", flag_on(), STR_CONVERTER_BOOL,
                         { atomic_config()->enabled = is_on(v); })
