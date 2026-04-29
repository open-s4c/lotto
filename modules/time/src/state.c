#include "state.h"
#include <lotto/engine/statemgr.h>
#include <lotto/sys/logger.h>

static time_config_t _config = {.enabled = true};

REGISTER_CONFIG(_config, {
    logger_infof("enabled = %s\n", _config.enabled ? "on" : "off");
})

time_config_t *
time_config(void)
{
    return &_config;
}
