#define LOGGER_BLOCK  LOGGER_CUR_BLOCK
#include <lotto/brokers/statemgr.h>
#include <lotto/modules/termination/state.h>
#include <lotto/util/macros.h>

termination_config_t _config;
REGISTER_CONFIG(_config, {
    logger_infof("mode  = %s\n", termination_mode_str(_config.mode));
    logger_infof("limit = %lu\n", _config.limit);
})

termination_config_t *
termination_config()
{
    return &_config;
}

#define GEN_TERMINATION_MODE(mode) [TERMINATION_MODE_##mode] = #mode,
static const char *_termination_mode_map[] = {[TERMINATION_MODE_NONE] = "NONE",
                                              FOR_EACH_TERMINATION_MODE};
#undef GEN_TERMINATION_MODE

BIT_STR(termination_mode, TERMINATION_MODE)
BIT_FROM(termination_mode, TERMINATION_MODE)
BIT_ALL_STR(termination_mode, TERMINATION_MODE)
