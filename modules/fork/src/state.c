#include "state.h"
#include <lotto/engine/statemgr.h>
#include <lotto/sys/logger.h>

static fork_execve_config_t _config;
REGISTER_CONFIG(_config, {
    _config.enabled = false;
    logger_infof("enabled = %s\n", _config.enabled ? "on" : "off");
})

fork_execve_config_t *
fork_execve_config()
{
    return &_config;
}
