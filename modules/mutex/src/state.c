#include "state.h"
#include <dice/module.h>
#include <lotto/engine/statemgr.h>
#include <lotto/sys/logger.h>

static mutex_config_t _config;

REGISTER_CONFIG(_config, {
    logger_infof("enabled        = %s\n", _config.enabled ? "on" : "off");
    logger_infof("deadlock_check = %s\n",
                 _config.deadlock_check ? "on" : "off");
})

mutex_config_t *
mutex_config()
{
    _config.enabled = true;
    return &_config;
}
