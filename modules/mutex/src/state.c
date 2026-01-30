/*
 */
#define LOGGER_PREFIX LOGGER_CUR_FILE
#define LOGGER_BLOCK  LOGGER_CUR_BLOCK
#include <lotto/brokers/statemgr.h>
#include <lotto/states/handlers/mutex.h>
#include <lotto/sys/logger_block.h>

static mutex_config_t _config;

REGISTER_CONFIG(_config, {
    logger_infof("enabled        = %s\n", _config.enabled ? "on" : "off");
    logger_infof("deadlock_check = %s\n",
                 _config.deadlock_check ? "on" : "off");
})

mutex_config_t *
mutex_config()
{
    _config.enabled = true;
    return &_config;
}
