#include "state.h"
#include <lotto/driver/flagmgr.h>
#include <lotto/driver/flags/modules.h>

REGISTER_RUNTIME_SWITCHABLE_CONFIG(region_filter_config(),
#ifdef QLOTTO_ENABLED
                                   true
#else
                                   false
#endif
)
