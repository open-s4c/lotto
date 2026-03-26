
#include "state.h"
#include <lotto/engine/pubsub.h>
#include <lotto/engine/statemgr.h>
#include <lotto/sys/logger.h>

static filtering_config_t _config;

REGISTER_CONFIG(_config, {
    logger_infof("enabled  = %s\n", _config.enabled ? "on" : "off");
    logger_infof("filename = %s\n", _config.filename);
})

filtering_config_t *
filtering_config()
{
    return &_config;
}
