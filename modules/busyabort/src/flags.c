#include "state.h"
#include <lotto/base/flags.h>
#include <lotto/driver/flagmgr.h>

NEW_PRETTY_CALLBACK_FLAG(HANDLER_BUSYABORT_ENABLED, "", "handler-busyabort",
                         "enable busy-loop abort handler", flag_on(),
                         STR_CONVERTER_BOOL,
                         { busyabort_config()->enabled = is_on(v); })

NEW_CALLBACK_FLAG(BUSYABORT_THRESHOLD, "", "busyabort-threshold", "",
                  "repeat count threshold before abort, 0 for never",
                  flag_uval(~0ULL),
                  { busyabort_config()->threshold = as_uval(v); })
