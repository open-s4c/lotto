/*
 */
#define LOGGER_PREFIX LOGGER_CUR_FILE
#define LOGGER_BLOCK  LOGGER_CUR_BLOCK
#include <lotto/brokers/statemgr.h>
#include <lotto/states/handlers/yield.h>
#include <lotto/sys/logger_block.h>

static yield_config_t _config;
REGISTER_CONFIG(_config, {
    logger_infof("enabled  = %s\n", _config.enabled ? "on" : "off");
    logger_infof("advisory = %s\n", _config.advisory ? "on" : "off");
})

yield_config_t *
yield_config()
{
    return &_config;
}
