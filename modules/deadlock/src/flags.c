#define LOGGER_BLOCK LOGGER_CUR_BLOCK
#include <lotto/cli/flagmgr.h>
#include <lotto/modules/deadlock/state.h>

NEW_PRETTY_CALLBACK_FLAG(HANDLER_DEADLOCK_ENABLED, "", "handler-deadlock",
                         "enable deadlock handler", flag_on(),
                         STR_CONVERTER_BOOL,
                         { deadlock_config()->enabled = is_on(v); })

NEW_CALLBACK_FLAG(HANDLER_DEADLOCK_LOST_RESOURCE_CHECK, "",
                  "check-lost-resource", "",
                  "enable deadlock handler's lost resource detection",
                  flag_on(),
                  { deadlock_config()->lost_resource_check = is_on(v); })

NEW_CALLBACK_FLAG(HANDLER_DEADLOCK_EXTRA_RELEASE_CHECK, "",
                  "check-extra-release", "",
                  "enable deadlock handler's extra release detection",
                  flag_on(),
                  { deadlock_config()->extra_release_check = is_on(v); })
