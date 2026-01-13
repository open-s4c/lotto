/*
 */
#define LOGGER_PREFIX LOGGER_CUR_FILE
#define LOGGER_BLOCK  LOGGER_CUR_BLOCK
#include <state.h>

#include <lotto/brokers/statemgr.h>
#include <lotto/sys/logger_block.h>

static priority_config_t _config;
REGISTER_CONFIG(_config, {
    logger_infof("enabled = %s\n", _config.enabled ? "on" : "off");
})

priority_config_t *
priority_config()
{
    return &_config;
}
