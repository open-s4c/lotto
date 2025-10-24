/*
 */
#define LOG_PREFIX LOG_CUR_FILE
#define LOG_BLOCK  LOG_CUR_BLOCK
#include <lotto/brokers/statemgr.h>
#include <lotto/states/handlers/mutex.h>
#include <lotto/sys/logger_block.h>

static mutex_config_t _config;

REGISTER_CONFIG(_config, {
    log_infof("enabled        = %s\n", _config.enabled ? "on" : "off");
    log_infof("deadlock_check = %s\n", _config.deadlock_check ? "on" : "off");
})

mutex_config_t *
mutex_config()
{
    return &_config;
}
