#include "state.h"
#include <lotto/engine/statemgr.h>
#include <lotto/sys/logger.h>

static memaccess_config_t _config = {.enabled = true};
REGISTER_CONFIG(_config, {
    _config.enabled = true;
    logger_infof("enabled = %s\n", _config.enabled ? "on" : "off");
})

memaccess_config_t *
memaccess_config()
{
    return &_config;
}
