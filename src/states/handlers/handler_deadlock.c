/*
 */
#define LOG_PREFIX LOG_CUR_FILE
#define LOG_BLOCK  LOG_CUR_BLOCK
#include <lotto/brokers/statemgr.h>
#include <lotto/states/handlers/deadlock.h>
#include <lotto/sys/logger_block.h>

static deadlock_config_t _config = {
    .enabled             = true,
    .extra_release_check = true,
    .lost_resource_check = true,
};

REGISTER_CONFIG(_config, {
    log_infof("enabled             = %s\n", _config.enabled ? "on" : "off");
    log_infof("lost_resource_check = %s\n",
              _config.lost_resource_check ? "on" : "off");
    log_infof("extra_release_check = %s\n",
              _config.extra_release_check ? "on" : "off");
})

deadlock_config_t *
deadlock_config()
{
    return &_config;
}
