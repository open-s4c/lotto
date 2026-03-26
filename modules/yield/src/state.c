#include "state.h"
#include <dice/module.h>
#include <lotto/engine/statemgr.h>
#include <lotto/sys/logger.h>

static yield_config_t _config;
REGISTER_CONFIG(_config, {
    logger_infof("enabled  = %s\n", _config.enabled ? "on" : "off");
    logger_infof("advisory = %s\n", _config.advisory ? "on" : "off");
})

yield_config_t *
yield_config()
{
    _config.enabled = true;
    return &_config;
}
