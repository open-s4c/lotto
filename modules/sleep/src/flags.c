#include "state.h"
#include <lotto/driver/flagmgr.h>

static void
_sleep_mode_help(char *dst)
{
    sleep_mode_all_str(dst);
}

NEW_PRETTY_CALLBACK_FLAG(SLEEP_MODE, "", LOTTO_MODULE_FLAG("mode"),
                         "sleep wrapper behavior MODE once|until",
                         flag_uval(SLEEP_MODE_UNTIL),
                         STR_CONVERTER_GET(sleep_mode_str, sleep_mode_from,
                                           sizeof("once|until"),
                                           _sleep_mode_help),
                         { sleep_config()->mode = as_uval(v); })
