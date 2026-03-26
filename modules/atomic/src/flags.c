#include "state.h"
#include <lotto/driver/flagmgr.h>

NEW_PRETTY_CALLBACK_FLAG(HANDLER_ATOMIC_ENABLED, "", "handler-atomic",
                         "enable atomic handler", flag_on(), STR_CONVERTER_BOOL,
                         { atomic_config()->enabled = is_on(v); })
