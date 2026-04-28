#include "state.h"
#include <lotto/engine/statemgr.h>
#include <lotto/sys/logger.h>

static uafcheck_config_t _config = {
    .max_pages = UAFCHECK_DEFAULT_MAX_PAGES,
    .prob      = UAFCHECK_DEFAULT_PROB,
};

REGISTER_CONFIG(_config, {
    logger_infof("max_pages = %lu\n", _config.max_pages);
    logger_infof("prob      = %g\n", _config.prob);
})

uafcheck_config_t *
uafcheck_config(void)
{
    return &_config;
}
