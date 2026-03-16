#define LOGGER_BLOCK LOGGER_CUR_BLOCK
#include "state.h"
#include <lotto/engine/statemgr.h>
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
