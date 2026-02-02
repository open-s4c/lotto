#include <string.h>

#define LOGGER_PREFIX LOGGER_CUR_FILE
#define LOGGER_BLOCK  LOGGER_CUR_BLOCK
#include <lotto/brokers/pubsub.h>
#include <lotto/brokers/statemgr.h>
#include <lotto/modules/filtering/state.h>
#include <lotto/sys/logger_block.h>

static filtering_config_t _config;

REGISTER_CONFIG(_config, {
    logger_infof("enabled  = %s\n", _config.enabled ? "on" : "off");
    logger_infof("filename = %s\n", _config.filename);
})

filtering_config_t *
filtering_config()
{
    return &_config;
}
