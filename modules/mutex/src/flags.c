#include "state.h"
#include <lotto/driver/flagmgr.h>
#include <lotto/engine/pubsub.h>
#include <lotto/modules/mutex/events.h>

NEW_CALLBACK_FLAG(HANDLER_MUTEX_DEADLOCK_CHECK, "",
                  LOTTO_MODULE_FLAG("check-deadlock"), "",
                  "enable mutex handler's deadlock detection", flag_off(),
                  { mutex_config()->deadlock_check = is_on(v); })

NEW_CALLBACK_FLAG(HANDLER_MUTEX_STRICT, "", LOTTO_MODULE_FLAG("strict"), "",
                  "abort on invalid mutex operations", flag_off(),
                  { mutex_config()->strict = is_on(v); })
