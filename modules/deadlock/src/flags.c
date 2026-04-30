#include "state.h"
#include <lotto/driver/flagmgr.h>
#include <lotto/driver/flags/modules.h>

REGISTER_RUNTIME_SWITCHABLE_CONFIG(deadlock_config(), false)

NEW_CALLBACK_FLAG(HANDLER_DEADLOCK_LOST_RESOURCE_CHECK, "",
                  LOTTO_MODULE_FLAG("check-lost-resource"), "",
                  "enable deadlock handler's lost resource detection",
                  flag_on(),
                  { deadlock_config()->lost_resource_check = is_on(v); })

NEW_CALLBACK_FLAG(HANDLER_DEADLOCK_EXTRA_RELEASE_CHECK, "",
                  LOTTO_MODULE_FLAG("check-extra-release"), "",
                  "enable deadlock handler's extra release detection",
                  flag_on(),
                  { deadlock_config()->extra_release_check = is_on(v); })
