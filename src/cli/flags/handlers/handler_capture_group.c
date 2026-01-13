/*
 */
#define LOGGER_PREFIX LOGGER_CUR_FILE
#define LOGGER_BLOCK  LOGGER_CUR_BLOCK
#include <lotto/cli/flagmgr.h>
#include <lotto/states/handlers/capture_group.h>

NEW_CALLBACK_FLAG(
    DELAYED_FUNCTIONS, "", "delayed-functions", "LIST(NAME:[0|1]:[0|1])",
    "functions to be delayed, a comma separated list of colon separated "
    "function names, whether the return value is relevant, whether the "
    "written memory is relevant",
    flag_sval(""), { strcpy(capture_group_config()->functions, as_sval(v)); })
NEW_CALLBACK_FLAG(DELAYED_CALLS, "", "delayed-calls", "INT",
                  "function call number to be delayed", flag_uval(0),
                  { capture_group_config()->group_threshold = as_uval(v); })
NEW_PUBLIC_CALLBACK_FLAG(DELAYED_ATOMIC, "", "delayed-atomic", "",
                         "delayed functions should be executed atomically",
                         flag_off(),
                         { capture_group_config()->atomic = is_on(v); })
FLAG_GETTER(delayed_atomic, DELAYED_ATOMIC)
