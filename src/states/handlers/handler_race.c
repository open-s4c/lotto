#define LOG_PREFIX LOG_CUR_FILE
#define LOG_BLOCK  LOG_CUR_BLOCK
#include <lotto/brokers/statemgr.h>
#include <lotto/states/handlers/race.h>
#include <lotto/sys/logger_block.h>

static race_config_t _config = {
    .enabled       = true,
    .abort_on_race = true,
};

REGISTER_CONFIG(_config, {
    log_infof("enabled            = %s\n", _config.enabled ? "on" : "off");
    log_infof("ignore_write_write = %s\n",
              _config.ignore_write_write ? "on" : "off");
    log_infof("only_write_ichpt   = %s\n",
              _config.only_write_ichpt ? "on" : "off");
    log_infof("abort_on_race      = %s\n",
              _config.abort_on_race ? "on" : "off");
})

race_config_t *
race_config()
{
    return &_config;
}
