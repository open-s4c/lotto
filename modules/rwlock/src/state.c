#define LOGGER_BLOCK LOGGER_CUR_BLOCK
#include <dice/module.h>
#include <lotto/engine/statemgr.h>
#include "state.h"
#include <lotto/sys/logger_block.h>

static rwlock_config_t _config;

REGISTER_CONFIG(_config, {
    logger_infof("enabled        = %s\n", _config.enabled ? "on" : "off");
})

rwlock_config_t *
rwlock_config()
{
    return &_config;
}
