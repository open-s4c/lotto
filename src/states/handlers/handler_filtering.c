/*
 */

#include <string.h>

#define LOG_PREFIX LOG_CUR_FILE
#define LOG_BLOCK  LOG_CUR_BLOCK
#include <lotto/brokers/pubsub.h>
#include <lotto/brokers/statemgr.h>
#include <lotto/states/handlers/filtering.h>
#include <lotto/sys/logger_block.h>

static filtering_config_t _config;

REGISTER_CONFIG(_config, {
    log_infof("enabled  = %s\n", _config.enabled ? "on" : "off");
    log_infof("filename = %s\n", _config.filename);
})

filtering_config_t *
filtering_config()
{
    return &_config;
}
