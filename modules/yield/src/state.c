#define LOGGER_PREFIX LOGGER_CUR_FILE
#define LOGGER_BLOCK  LOGGER_CUR_BLOCK
#include <dice/module.h>
#include <lotto/brokers/statemgr.h>
#include <lotto/modules/yield/state.h>
#include <lotto/sys/logger_block.h>

static yield_config_t _config;
REGISTER_CONFIG(_config, {
    logger_infof("enabled  = %s\n", _config.enabled ? "on" : "off");
    logger_infof("advisory = %s\n", _config.advisory ? "on" : "off");
})

yield_config_t *
yield_config()
{
    _config.enabled = true;
    return &_config;
}
