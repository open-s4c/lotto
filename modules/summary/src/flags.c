#include "state.h"
#include <lotto/driver/flagmgr.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/string.h>

NEW_PUBLIC_CALLBACK_FLAG(SUMMARY_CSV, "", "summary-csv", "FILE",
                         "write summary output as CSV to FILE", flag_sval(""), {
                             ASSERT(sys_strlen(as_sval(v)) < PATH_MAX);
                             sys_strcpy(summary_config()->csv, as_sval(v));
                         })
