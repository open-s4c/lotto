#include "state.h"
#include <dice/module.h>
#include <lotto/engine/statemgr.h>
#include <lotto/sys/logger.h>

static watchdog_config_t _config;

REGISTER_CONFIG(_config, {
    logger_infof("enabled = %s\n", _config.enabled ? "on" : "off");
    logger_infof("budget  = %lu\n", _config.budget);
})

watchdog_config_t *
watchdog_config()
{
    return &_config;
}
