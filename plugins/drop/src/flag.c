/*
 */
#define LOG_PREFIX LOG_CUR_FILE
#define LOG_BLOCK  LOG_CUR_BLOCK
#include <state.h>

#include <lotto/cli/flagmgr.h>

NEW_PRETTY_CALLBACK_FLAG(HANDLER_DROP_ENABLED, "", "handler-drop",
                         "enable drop handler", flag_on(), STR_CONVERTER_BOOL,
                         { drop_config()->enabled = is_on(v); })
