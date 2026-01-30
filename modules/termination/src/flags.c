/*
 */
#define LOGGER_PREFIX LOGGER_CUR_FILE
#define LOGGER_BLOCK  LOGGER_CUR_BLOCK
#include <lotto/cli/flagmgr.h>
#include <lotto/states/handlers/termination.h>

static void
_termination_mode_help(char *dst)
{
    termination_mode_all_str(dst);
}

NEW_PUBLIC_PRETTY_CALLBACK_FLAG(
    TERMINATION_TYPE, "", "termination-type", "termination condition type",
    flag_uval(TERMINATION_MODE_NONE),
    STR_CONVERTER_GET(termination_mode_str, termination_mode_from,
                      TERMINATION_MODE_MAX_STR_LEN, _termination_mode_help),
    { termination_config()->mode = as_uval(v); })
NEW_PUBLIC_CALLBACK_FLAG(
    TERMINATION_LIMIT, "", "termination-limit", "INT",
    "termination limit, interpreted according to the termination type",
    flag_uval(0), { termination_config()->limit = as_uval(v); })

flag_t
flag_termination_type()
{
    return FLAG_TERMINATION_TYPE;
}

flag_t
flag_termination_limit()
{
    return FLAG_TERMINATION_LIMIT;
}
