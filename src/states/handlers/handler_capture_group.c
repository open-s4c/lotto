/*
 */
#define LOG_PREFIX LOG_CUR_FILE
#define LOG_BLOCK  LOG_CUR_BLOCK
#include <lotto/brokers/statemgr.h>
#include <lotto/states/handlers/capture_group.h>
#include <lotto/sys/logger_block.h>

static capture_group_config_t _config;
REGISTER_CONFIG(_config, {
    log_infof("atomic                = %s\n", _config.atomic ? "on" : "off");
    log_infof("check                 = %s\n", _config.check ? "on" : "off");
    log_infof("path                  = %s\n", _config.path);
    log_infof("group_threshold       = %lu\n", _config.group_threshold);
    log_infof("functions             = %s\n", _config.functions);
})

capture_group_config_t *
capture_group_config()
{
    return &_config;
}
