/*
 */
#define LOGGER_PREFIX LOGGER_CUR_FILE
#define LOGGER_BLOCK  LOGGER_CUR_BLOCK
#include <lotto/brokers/statemgr.h>
#include <lotto/sys/logger_block.h>

/*******************************************************************************
 * config
 ******************************************************************************/
typedef struct pos_config {
    marshable_t m;
    uint64_t wd_threshold;
    uint64_t wd_divisor;
    bool enabled;
} pos_config_t;
static pos_config_t _config;
REGISTER_CONFIG(_config, {
    logger_infof("wd_threshold = %lu\n", _config.wd_threshold);
    logger_infof("wd_divisor   = %lu\n", _config.wd_divisor);
    logger_infof("enabled      = %s\n", _config.enabled ? "on" : "off");
})

pos_config_t *
pos_config()
{
    return &_config;
}
