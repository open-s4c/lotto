#include "state.h"
#include <dice/module.h>
#include <lotto/engine/statemgr.h>
#include <lotto/sys/logger.h>

static rwlock_config_t _config;

REGISTER_CONFIG(_config, {})

rwlock_config_t *
rwlock_config()
{
    return &_config;
}
