#define LOGGER_BLOCK LOGGER_CUR_BLOCK
#include <lotto/brokers/statemgr.h>
#include <lotto/modules/race/state.h>
#include <lotto/sys/logger_block.h>

static race_config_t _config;

REGISTER_CONFIG(_config, {
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
