/*
 */
#define LOGGER_PREFIX LOGGER_CUR_FILE
#define LOGGER_BLOCK  LOGGER_CUR_BLOCK
#include <lotto/cli/flagmgr.h>
#include <lotto/states/handlers/race.h>

NEW_PRETTY_CALLBACK_FLAG(HANDLER_RACE_ENABLED, "", "handler-race",
                         "enable data race detection handler", flag_on(),
                         STR_CONVERTER_BOOL, {
                             race_config()->enabled = is_on(v);
#ifndef RACE_DEFAULT
                             race_config()->ignore_write_write = true;
                             race_config()->only_write_ichpt   = true;
#endif
                         })
NEW_CALLBACK_FLAG(HANDLER_RACE_STRICT, "", "handler-race-strict", "",
                  "abort when data race detected", flag_off(),
                  { race_config()->abort_on_race = is_on(v); })
