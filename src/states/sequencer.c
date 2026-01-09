#define LOG_PREFIX LOG_CUR_FILE
#define LOG_BLOCK  LOG_CUR_BLOCK
#include <lotto/brokers/statemgr.h>
#include <lotto/states/sequencer.h>
#include <lotto/sys/logger_block.h>

static sequencer_config_t _config;
REGISTER_CONFIG(_config, {
    char gran_str[RECORD_GRANULARITIES_MAX_LEN];
    record_granularities_str(_config.gran, gran_str);
    logger_infof("gran  = %s\n", gran_str);
    logger_infof("slack = %lu\n", _config.slack);
    logger_infof("stable_address_method = %lu\n", _config.stable_address_method);
})

sequencer_config_t *
sequencer_config()
{
    return &_config;
}