#include "state.h"
#include <dice/module.h>
#include <lotto/engine/statemgr.h>
#include <lotto/sys/logger.h>

static priority_config_t _config = {.enabled = true};
REGISTER_CONFIG(_config, {
    _config.enabled = true;
    logger_infof("enabled = %s\n", _config.enabled ? "on" : "off");
})

priority_config_t *
priority_config()
{
    return &_config;
}
