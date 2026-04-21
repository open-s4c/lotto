#include "state.h"
#include <lotto/driver/flagmgr.h>
#include <lotto/driver/flags/modules.h>
#include <lotto/sys/string.h>

static void
_filtering_enabled(bool enabled)
{
    filtering_config()->enabled = enabled;
}

_FLAGMGR_SUBSCRIBE({ register_runtime_switchable_module(LOTTO_MODULE_NAME,
                                                        _filtering_enabled); })

NEW_CALLBACK_FLAG(FILTERING, "F", "filtering-on", "",
                  "enables filtering of events",
#ifdef QLOTTO_ENABLED
                  flag_on(),
#else
            flag_off(),
#endif
                  { filtering_config()->enabled = is_on(v); })
NEW_CALLBACK_FLAG(FILTERING_CONFIG, "", LOTTO_MODULE_FLAG("config"), "FILE",
                  "filtering configuration file (containing key=value lines)",
                  flag_sval("filtering.conf"), {
                      ASSERT(sys_strlen(as_sval(v)) < PATH_MAX);
                      strcpy(filtering_config()->filename, as_sval(v));
                  })
