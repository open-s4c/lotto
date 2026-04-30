#include "state.h"
#include <lotto/engine/statemgr.h>

static busyabort_config_t _config;

REGISTER_CONFIG(_config, {
    _config.enabled   = false;
    _config.threshold = 1000000;
})

busyabort_config_t *
busyabort_config(void)
{
    return &_config;
}
