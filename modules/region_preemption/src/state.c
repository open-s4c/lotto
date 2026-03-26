#include "state.h"
#include <lotto/engine/statemgr.h>
#include <lotto/sys/logger.h>

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
