/*
 */
#define LOG_PREFIX LOG_CUR_FILE
#define LOG_BLOCK  LOG_CUR_BLOCK
#include <lotto/cli/flagmgr.h>
#include <lotto/states/handlers/region_filter.h>

NEW_PRETTY_CALLBACK_FLAG(REGION_FILTER, "", "handler-region-filter",
                         "enable region filter handler",
#ifdef QLOTTO_ENABLED
                         flag_on(),
#else
            flag_off(),
#endif
                         STR_CONVERTER_BOOL,
                         { region_filter_config()->enabled = is_on(v); })
