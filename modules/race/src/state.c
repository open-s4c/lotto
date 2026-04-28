#include "state.h"
#include <lotto/engine/statemgr.h>
#include <lotto/sys/logger.h>

static race_config_t _config = {.enabled = true};

REGISTER_CONFIG(_config, {
    _config.enabled = true;
    logger_infof("enabled            = %s\n", _config.enabled ? "on" : "off");
    logger_infof("ignore_write_write = %s\n",
                 _config.ignore_write_write ? "on" : "off");
    logger_infof("only_write_ichpt   = %s\n",
                 _config.only_write_ichpt ? "on" : "off");
    logger_infof("abort_on_race      = %s\n",
                 _config.abort_on_race ? "on" : "off");
})

race_config_t *
race_config()
{
    return &_config;
}
