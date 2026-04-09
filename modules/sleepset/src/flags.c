#include "state.h"
#include <lotto/driver/flagmgr.h>

NEW_CALLBACK_FLAG(SLEEPSET, "", "sleepset-on", "",
                  "enable sleepset-based scheduling reduction", flag_off(),
                  { sleepset_config()->enabled = is_on(v); })
