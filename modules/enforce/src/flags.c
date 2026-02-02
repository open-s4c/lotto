#define LOGGER_PREFIX LOGGER_CUR_FILE
#define LOGGER_BLOCK  LOGGER_CUR_BLOCK
#include <lotto/cli/flagmgr.h>
#include <lotto/states/handlers/enforce.h>

static void
_enforce_modes_help(char *dst)
{
    enforce_modes_str(ENFORCE_MODE_ANY, dst);
}

NEW_PRETTY_CALLBACK_FLAG(
    ENFORCEMENT_MODE, "E", "enforcement-mode", "enables replay enforcement",
    flag_uval(ENFORCE_MODE_CAT | ENFORCE_MODE_TID | ENFORCE_MODE_CUSTOM |
              ENFORCE_MODE_SEED),
    STR_CONVERTER_PRINT(enforce_modes_str, enforce_modes_from,
                        ENFORCE_MODES_MAX_LEN, _enforce_modes_help),
    { enforce_config()->modes = as_uval(v); })
