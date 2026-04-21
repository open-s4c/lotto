#include "state.h"
#include <lotto/engine/statemgr.h>
#include <lotto/sys/logger.h>

static clock_config_t _config = {
    .base_inc  = 51,
    .mult_inc  = 1,
    .max_gap   = 10,
    .burst_gap = 30,
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
