#include <lotto/engine/statemgr.h>
#include <lotto/modules/terminate/state.h>
#include <lotto/util/macros.h>

terminate_config_t _config;
REGISTER_CONFIG(_config, {
    logger_infof("mode  = %s\n", terminate_mode_str(_config.mode));
    logger_infof("limit = %lu\n", _config.limit);
})

terminate_config_t *
terminate_config()
{
    return &_config;
}

#define GEN_TERMINATE_MODE(mode) [TERMINATE_MODE_##mode] = #mode,
static const char *_terminate_mode_map[] = {[TERMINATE_MODE_NONE] = "NONE",
                                            FOR_EACH_TERMINATE_MODE};
#undef GEN_TERMINATE_MODE

BIT_STR(terminate_mode, TERMINATE_MODE)
BIT_FROM(terminate_mode, TERMINATE_MODE)
BIT_ALL_STR(terminate_mode, TERMINATE_MODE)
