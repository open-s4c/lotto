/*
 */
#define LOG_PREFIX LOG_CUR_FILE
#define LOG_BLOCK  LOG_CUR_BLOCK
#include <lotto/cli/flagmgr.h>
#include <lotto/states/handlers/filtering.h>
#include <lotto/sys/string.h>

NEW_CALLBACK_FLAG(FILTERING, "F", "filtering-on", "",
                  "enables filtering of events",
#ifdef QLOTTO_ENABLED
                  flag_on(),
#else
            flag_off(),
#endif
                  { filtering_config()->enabled = is_on(v); })
NEW_CALLBACK_FLAG(FILTERING_CONFIG, "", "filtering-config", "FILE",
                  "filtering configuration file (containing key=value lines)",
                  flag_sval("filtering.conf"), {
                      ASSERT(sys_strlen(as_sval(v)) < PATH_MAX);
                      strcpy(filtering_config()->filename, as_sval(v));
                  })
