#include <stdlib.h>

#include "state.h"
#include <lotto/driver/flagmgr.h>
#include <lotto/driver/flags/modules.h>

#define DEFAULT_BUDGET 100

REGISTER_RUNTIME_SWITCHABLE_CONFIG(watchdog_config(), true)
