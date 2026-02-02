#define LOGGER_BLOCK LOGGER_CUR_BLOCK
#include <lotto/brokers/statemgr.h>
#include <lotto/modules/region_preemption/state.h>
#include <lotto/sys/logger_block.h>

/*******************************************************************************
 * config
 ******************************************************************************/
static region_preemption_config_t _config;
REGISTER_CONFIG(_config, {
    logger_infof("default = %s\n", _config.default_on ? "on" : "off");
})

region_preemption_config_t *
region_preemption_config()
{
    return &_config;
}
