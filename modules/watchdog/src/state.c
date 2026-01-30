/*
 */
#define LOGGER_PREFIX LOGGER_CUR_FILE
#define LOGGER_BLOCK  LOGGER_CUR_BLOCK
#include <lotto/brokers/statemgr.h>
#include <lotto/states/handlers/watchdog.h>
#include <lotto/sys/logger_block.h>

static watchdog_config_t _config;

REGISTER_CONFIG(_config, {
    logger_infof("enabled = %s\n", _config.enabled ? "on" : "off");
    logger_infof("budget  = %lu\n", _config.budget);
})

watchdog_config_t *
watchdog_config()
{
    return &_config;
}
