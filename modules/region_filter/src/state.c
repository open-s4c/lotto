#define LOGGER_PREFIX LOGGER_CUR_FILE
#define LOGGER_BLOCK  LOGGER_CUR_BLOCK
#include <lotto/brokers/statemgr.h>
#include <lotto/modules/region_filter/state.h>
#include <lotto/sys/logger_block.h>

static region_filter_config_t _config;
REGISTER_CONFIG(_config, {
    logger_infof("enabled = %s\n", _config.enabled ? "on" : "off");
})

region_filter_config_t *
region_filter_config()
{
    return &_config;
}
