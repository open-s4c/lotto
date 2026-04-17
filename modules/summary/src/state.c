#include "state.h"
#include <lotto/engine/statemgr.h>
#include <lotto/sys/logger.h>

static summary_config_t _config;

REGISTER_CONFIG(_config, {
    logger_infof("csv = %s\n", _config.csv[0] ? _config.csv : "<disabled>");
})

summary_config_t *
summary_config()
{
    return &_config;
}
