/*
 */
#define LOG_PREFIX LOG_CUR_FILE
#define LOG_BLOCK  LOG_CUR_BLOCK
#include <lotto/cli/flagmgr.h>
#include <lotto/states/handlers/mutex.h>

NEW_PRETTY_CALLBACK_FLAG(HANDLER_MUTEX_ENABLED, "", "handler-mutex",
                         "enable mutex handler", flag_on(), STR_CONVERTER_BOOL,
                         { mutex_config()->enabled = is_on(v); })
NEW_CALLBACK_FLAG(HANDLER_MUTEX_DEADLOCK_CHECK, "", "check-deadlock", "",
                  "enable mutex handler's deadlock detection", flag_off(),
                  { mutex_config()->deadlock_check = is_on(v); })
