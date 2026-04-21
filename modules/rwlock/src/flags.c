#include "state.h"
#include <lotto/driver/flagmgr.h>
#include <lotto/driver/flags/modules.h>

static void
_rwlock_enabled(bool enabled)
{
    rwlock_config()->enabled = enabled;
}

_FLAGMGR_SUBSCRIBE({ register_runtime_switchable_module(LOTTO_MODULE_NAME,
                                                        _rwlock_enabled); })

NEW_PRETTY_CALLBACK_FLAG(HANDLER_RWLOCK_ENABLED, "", "handler-rwlock",
                         "enable rwlock handler", flag_on(), STR_CONVERTER_BOOL,
                         { rwlock_config()->enabled = is_on(v); })
