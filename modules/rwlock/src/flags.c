#include "state.h"
#include <lotto/driver/flagmgr.h>

NEW_PRETTY_CALLBACK_FLAG(HANDLER_RWLOCK_ENABLED, "", "handler-rwlock",
                         "enable rwlock handler", flag_on(), STR_CONVERTER_BOOL,
                         { rwlock_config()->enabled = is_on(v); })
