#include <lotto/engine/statemgr.h>
#include <lotto/modules/impasse/state.h>
#include <lotto/sys/logger.h>

static impasse_config_t _config = {.enabled = true};
REGISTER_CONFIG(_config, {
    _config.enabled = true;
    logger_infof("enabled = %s\n", _config.enabled ? "on" : "off");
})

impasse_config_t *
impasse_config()
{
    return &_config;
}
