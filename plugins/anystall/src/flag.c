/*
 */
#define LOGGER_PREFIX LOGGER_CUR_FILE
#define LOGGER_BLOCK  LOGGER_CUR_BLOCK
#include <state.h>

#include <lotto/cli/flagmgr.h>

NEW_PRETTY_CALLBACK_FLAG(HANDLER_ANYSTALL_ENABLED, "", "handler-anystall",
                         "enable any task stalling handler", flag_off(),
                         STR_CONVERTER_BOOL,
                         { anystall_config()->enabled = is_on(v); })

NEW_CALLBACK_FLAG(HANDLER_ANYSTALL_LIMIT, "", "handler-anystall-limit", "INT",
                  "limit for any task stalling detection", flag_uval(1000),
                  { anystall_config()->anytask_limit = as_uval(v); })
