#define LOGGER_BLOCK LOGGER_CUR_BLOCK
#include "state.h"
#include <lotto/engine/statemgr.h>
#include <lotto/sys/logger_block.h>

static inactivity_timeout_config_t _config;
REGISTER_CONFIG(_config, { logger_infof("alarm = %lu\n", _config.alarm); })

inactivity_timeout_config_t *
inactivity_timeout_config()
{
    return &_config;
}
