#include "state.h"
#include <lotto/engine/statemgr.h>

static busyabort_config_t _config;

REGISTER_CONFIG(_config, {
    _config.enabled   = true;
    _config.threshold = ~0ULL;
})

busyabort_config_t *
busyabort_config(void)
{
    return &_config;
}
