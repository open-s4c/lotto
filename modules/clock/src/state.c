#include "state.h"
#include <lotto/engine/statemgr.h>
#include <lotto/sys/logger.h>

static clock_config_t _config = {
    .base_inc  = 1,
    .mult_inc  = 100000,
    .max_gap   = 1,
    .burst_gap = 1,
};

REGISTER_CONFIG(_config, {
    logger_infof("base_inc = %lu\n", _config.base_inc);
    logger_infof("mult_inc = %lu\n", _config.mult_inc);
    logger_infof("max_gap = %lu\n", _config.max_gap);
    logger_infof("burst_gap = %lu\n", _config.burst_gap);
})

clock_config_t *
clock_config(void)
{
    return &_config;
}
