/*
 */
#define LOG_PREFIX LOG_CUR_FILE
#define LOG_BLOCK  LOG_CUR_BLOCK
#include <lotto/cli/flagmgr.h>
#include <lotto/states/handlers/region_preemption.h>

NEW_PRETTY_CALLBACK_FLAG(
    HANDLER_REGION_PREEMPTION_ENABLED, "", "region-preemption",
    "avoid context switches unless inside a preemption region", flag_on(),
    STR_CONVERTER_BOOL, { region_preemption_config()->enabled = is_on(v); })
NEW_CALLBACK_FLAG(REGION_PREEMPTION_DEFAULT_OFF, "", "preemptions-off", "",
                  "disables preemptions", flag_off(),
                  { region_preemption_config()->default_on = is_off(v); })
