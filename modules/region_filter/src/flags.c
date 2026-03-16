#define LOGGER_BLOCK LOGGER_CUR_BLOCK
#include <lotto/driver/flagmgr.h>
#include "state.h"

NEW_PRETTY_CALLBACK_FLAG(REGION_FILTER, "", "handler-region-filter",
                         "enable region filter handler",
#ifdef QLOTTO_ENABLED
                         flag_on(),
#else
            flag_off(),
#endif
                         STR_CONVERTER_BOOL,
                         { region_filter_config()->enabled = is_on(v); })
