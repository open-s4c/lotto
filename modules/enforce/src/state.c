#define LOGGER_PREFIX LOGGER_CUR_FILE
#define LOGGER_BLOCK  LOGGER_CUR_BLOCK
#include <lotto/base/marshable.h>
#include <lotto/base/string.h>
#include <lotto/brokers/catmgr.h>
#include <lotto/brokers/statemgr.h>
#include <lotto/modules/enforce/state.h>
#include <lotto/sys/logger_block.h>
#include <lotto/util/macros.h>

/*******************************************************************************
 * config
 ******************************************************************************/

static enforce_config_t _config;
REGISTER_CONFIG(_config, {
    char str[ENFORCE_MODES_MAX_LEN];
    enforce_modes_str(_config.modes, str);
    logger_infof("modes           = %s\n", str);
    logger_infof("compare_address = %s\n", _config.compare_address ? "on" : "off");
})

enforce_config_t *
enforce_config()
{
    return &_config;
}

/*******************************************************************************
 * state
 ******************************************************************************/
static enforce_state_t _state;

static void
_printm(const marshable_t *m)
{
    logger_infof("id:   %lu\n", _state.ctx.id);
    logger_infof("cat:  %s\n", category_str(_state.ctx.cat));
    logger_infof("pc:   %p\n", (void *)_state.ctx.pc);
    logger_infof("seed: %lu\n", _state.seed);
}
REGISTER_STATE(PERSISTENT, _state, {
    _state.m       = MARSHABLE_STATIC(sizeof(_state));
    _state.m.print = _printm;
})

enforce_state_t *
enforce_state()
{
    return &_state;
}

/*******************************************************************************
 * enforcement modes
 ******************************************************************************/

#define GEN_ENFORCE_MODE(mode) [ENFORCE_MODE_##mode] = #mode,
static const char *_enforce_mode_map[] = {FOR_EACH_ENFORCE_MODE};
#undef GEN_ENFORCE_MODE

BITS_STR(enforce_mode, enforce_modes)
BITS_FROM(enforce_mode, enforce_modes, ENFORCE_MODE, ENFORCE_MODES)
BITS_HAS(enforce_mode, enforce_modes)
