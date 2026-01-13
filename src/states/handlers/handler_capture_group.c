/*
 */
#define LOGGER_PREFIX LOGGER_CUR_FILE
#define LOGGER_BLOCK  LOGGER_CUR_BLOCK
#include <lotto/brokers/statemgr.h>
#include <lotto/states/handlers/capture_group.h>
#include <lotto/sys/logger_block.h>

static capture_group_config_t _config;
REGISTER_CONFIG(_config, {
    logger_infof("atomic                = %s\n", _config.atomic ? "on" : "off");
    logger_infof("check                 = %s\n", _config.check ? "on" : "off");
    logger_infof("path                  = %s\n", _config.path);
    logger_infof("group_threshold       = %lu\n", _config.group_threshold);
    logger_infof("functions             = %s\n", _config.functions);
})

capture_group_config_t *
capture_group_config()
{
    return &_config;
}
