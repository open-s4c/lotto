/*
 */
#define LOG_PREFIX LOG_CUR_FILE
#define LOG_BLOCK  LOG_CUR_BLOCK
#include <lotto/brokers/statemgr.h>
#include <lotto/states/handlers/region_preemption.h>
#include <lotto/sys/logger_block.h>

/*******************************************************************************
 * config
 ******************************************************************************/
static region_preemption_config_t _config;
REGISTER_CONFIG(_config, {
    log_infof("default = %s\n", _config.default_on ? "on" : "off");
})

region_preemption_config_t *
region_preemption_config()
{
    return &_config;
}
