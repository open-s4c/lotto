#define LOG_PREFIX LOG_CUR_FILE
#define LOG_BLOCK  LOG_CUR_BLOCK
#include <lotto/brokers/statemgr.h>
#include <lotto/states/handlers/watchdog.h>
#include <lotto/sys/logger_block.h>

static watchdog_config_t _config;

REGISTER_CONFIG(_config, {
    log_infof("enabled = %s\n", _config.enabled ? "on" : "off");
    log_infof("budget  = %lu\n", _config.budget);
})

watchdog_config_t *
watchdog_config()
{
    _config.enabled = true;
    _config.budget  = 100;
    return &_config;
}
