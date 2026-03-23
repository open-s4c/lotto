#define LOGGER_BLOCK LOGGER_CUR_BLOCK
#include "state.h"
#include <lotto/engine/pubsub.h>
#include <lotto/driver/flagmgr.h>
#include <lotto/modules/mutex/events.h>

LOTTO_ADVERTISE_TYPE(EVENT_MUTEX_ACQUIRE)
LOTTO_ADVERTISE_TYPE(EVENT_MUTEX_TRYACQUIRE)
LOTTO_ADVERTISE_TYPE(EVENT_MUTEX_RELEASE)

NEW_PRETTY_CALLBACK_FLAG(HANDLER_MUTEX_ENABLED, "", "handler-mutex",
                         "enable mutex handler", flag_on(), STR_CONVERTER_BOOL,
                         { mutex_config()->enabled = is_on(v); })
NEW_CALLBACK_FLAG(HANDLER_MUTEX_DEADLOCK_CHECK, "", "check-deadlock", "",
                  "enable mutex handler's deadlock detection", flag_off(),
                  { mutex_config()->deadlock_check = is_on(v); })
