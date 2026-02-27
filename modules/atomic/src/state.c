#define LOGGER_BLOCK LOGGER_CUR_BLOCK
#include <lotto/brokers/statemgr.h>
#include <lotto/modules/atomic/state.h>
#include <lotto/sys/logger_block.h>

static atomic_config_t _config;
REGISTER_CONFIG(_config, {
    logger_infof("enabled = %s\n", _config.enabled ? "on" : "off");
})

atomic_config_t *
atomic_config()
{
    return &_config;
}
