#define LOGGER_BLOCK LOGGER_CUR_BLOCK
#include <lotto/driver/flagmgr.h>
#include "state.h"

NEW_PRETTY_CALLBACK_FLAG(HANDLER_RWLOCK_ENABLED, "", "handler-rwlock",
                         "enable rwlock handler", flag_on(), STR_CONVERTER_BOOL,
                         { rwlock_config()->enabled = is_on(v); })
