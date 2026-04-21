#include "state.h"
#include <lotto/driver/flagmgr.h>
#include <lotto/driver/flags/modules.h>
#include <lotto/engine/pubsub.h>
#include <lotto/modules/mutex/events.h>

static void
_mutex_enabled(bool enabled)
{
    mutex_config()->enabled = enabled;
}

_FLAGMGR_SUBSCRIBE({ register_runtime_switchable_module(LOTTO_MODULE_NAME,
                                                        _mutex_enabled); })

NEW_PRETTY_CALLBACK_FLAG(HANDLER_MUTEX_ENABLED, "", "handler-mutex",
                         "enable mutex handler", flag_on(), STR_CONVERTER_BOOL,
                         { mutex_config()->enabled = is_on(v); })
NEW_CALLBACK_FLAG(HANDLER_MUTEX_DEADLOCK_CHECK, "", LOTTO_MODULE_FLAG("check-deadlock"), "",
                  "enable mutex handler's deadlock detection", flag_off(),
                  { mutex_config()->deadlock_check = is_on(v); })
