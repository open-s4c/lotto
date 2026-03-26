#include "state.h"
#include <lotto/engine/statemgr.h>
#include <lotto/sys/logger.h>

static inactivity_config_t _config;
REGISTER_CONFIG(_config, { logger_infof("alarm = %lu\n", _config.alarm); })

inactivity_config_t *
inactivity_config()
{
    return &_config;
}
