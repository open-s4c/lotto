/*
 */
#define LOG_PREFIX LOG_CUR_FILE
#define LOG_BLOCK  LOG_CUR_BLOCK
#include <lotto/brokers/statemgr.h>
#include <lotto/states/handlers/atomic.h>
#include <lotto/sys/logger_block.h>

static atomic_config_t _config;
REGISTER_CONFIG(_config, {
    log_infof("enabled = %s\n", _config.enabled ? "on" : "off");
})

atomic_config_t *
atomic_config()
{
    return &_config;
}
