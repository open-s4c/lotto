#include "state.h"
#include <lotto/engine/statemgr.h>
#include <lotto/sys/logger.h>

static region_filter_config_t _config;
REGISTER_CONFIG(_config, {
#ifdef QLOTTO_ENABLED
    _config.enabled = true;
#else
    _config.enabled = false;
#endif
    logger_infof("enabled = %s\n", _config.enabled ? "on" : "off");
})

region_filter_config_t *
region_filter_config()
{
    return &_config;
}
