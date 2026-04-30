#include "state.h"
#include <lotto/driver/flagmgr.h>

NEW_PUBLIC_CALLBACK_FLAG(CLOCK_BASE_INC, "", "clock-base-inc", "UINT",
                         "clock fallback base increment", flag_uval(1),
                         { clock_config()->base_inc = as_uval(v); })

NEW_PUBLIC_CALLBACK_FLAG(CLOCK_MULT_INC, "", "clock-mult-inc", "UINT",
                         "clock fallback increment multiplier",
                         flag_uval(100000),
                         { clock_config()->mult_inc = as_uval(v); })

NEW_PUBLIC_CALLBACK_FLAG(CLOCK_MAX_GAP, "", "clock-max-gap", "UINT",
                         "clock fallback max normal gap before burst",
                         flag_uval(1),
                         { clock_config()->max_gap = as_uval(v); })

NEW_PUBLIC_CALLBACK_FLAG(CLOCK_MULT_BURST, "", "clock-mult-burst", "UINT",
                         "clock fallback burst multiplier", flag_uval(1),
                         { clock_config()->burst_gap = as_uval(v); })
