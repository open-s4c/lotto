#include <lotto/driver/flagmgr.h>
#include <lotto/modules/terminate/state.h>

static void
_terminate_mode_help(char *dst)
{
    terminate_mode_all_str(dst);
}

NEW_PUBLIC_PRETTY_CALLBACK_FLAG(
    TERMINATE_TYPE, "", "terminate-type", "termination condition type",
    flag_uval(TERMINATE_MODE_NONE),
    STR_CONVERTER_GET(terminate_mode_str, terminate_mode_from,
                      TERMINATE_MODE_MAX_STR_LEN, _terminate_mode_help),
    { terminate_config()->mode = as_uval(v); })
NEW_PUBLIC_CALLBACK_FLAG(
    TERMINATE_LIMIT, "", "terminate-limit", "INT",
    "termination limit, interpreted according to the termination type",
    flag_uval(0), { terminate_config()->limit = as_uval(v); })

flag_t
flag_terminate_type()
{
    return FLAG_TERMINATE_TYPE;
}

flag_t
flag_terminate_limit()
{
    return FLAG_TERMINATE_LIMIT;
}
