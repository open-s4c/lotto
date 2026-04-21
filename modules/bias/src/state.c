#include "state.h"
#include <dice/module.h>
#include <lotto/engine/pubsub.h>
#include <lotto/engine/statemgr.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger.h>
#include <lotto/sys/string.h>

static void
_bias_config_print(const marshable_t *m)
{
    const bias_config_t *cfg = (const bias_config_t *)m;
    logger_infof("initial_policy = %s\n", bias_policy_str(cfg->initial_policy));
    logger_infof("toggle_policy = %s\n", bias_policy_str(cfg->toggle_policy));
}

static void
_bias_state_print(const marshable_t *m)
{
    const bias_state_t *state = (const bias_state_t *)m;
    logger_infof("current_policy = %s\n",
                 bias_policy_str(state->current_policy));
    logger_infof("toggle_policy = %s\n", bias_policy_str(state->toggle_policy));
}

static bias_config_t _config = {
    .initial_policy = BIAS_POLICY_NONE,
    .toggle_policy  = BIAS_POLICY_CURRENT,
};
REGISTER_CONFIG_NONSTATIC(_config,
                          MARSHABLE_STATIC_PRINTABLE(sizeof(_config),
                                                     _bias_config_print))

static bias_state_t _state;
REGISTER_PERSISTENT(_state, {
    _state.m = MARSHABLE_STATIC_PRINTABLE(sizeof(_state), _bias_state_print);
    _state.current_policy = BIAS_POLICY_NONE;
    _state.toggle_policy  = BIAS_POLICY_CURRENT;
})

ON_INITIALIZATION_PHASE({
    _state.current_policy = _config.initial_policy;
    _state.toggle_policy  = _config.toggle_policy;
})

bias_config_t *
bias_config(void)
{
    return &_config;
}

bias_state_t *
bias_state(void)
{
    return &_state;
}

const char *
bias_policy_str(uint64_t bits)
{
    switch ((bias_policy_t)bits) {
        case BIAS_POLICY_NONE:
            return "NONE";
        case BIAS_POLICY_CURRENT:
            return "CURRENT";
        case BIAS_POLICY_LOWEST:
            return "LOWEST";
        case BIAS_POLICY_HIGHEST:
            return "HIGHEST";
        default:
            ASSERT(false && "unexpected bias policy");
            return "NONE";
    }
}

uint64_t
bias_policy_from(const char *src)
{
    if (sys_strcmp(src, "NONE") == 0)
        return BIAS_POLICY_NONE;
    if (sys_strcmp(src, "CURRENT") == 0)
        return BIAS_POLICY_CURRENT;
    if (sys_strcmp(src, "LOWEST") == 0)
        return BIAS_POLICY_LOWEST;
    if (sys_strcmp(src, "HIGHEST") == 0)
        return BIAS_POLICY_HIGHEST;
    ASSERT(false && "invalid bias policy");
    return BIAS_POLICY_NONE;
}

void
bias_policy_all_str(char *dst)
{
    sys_strcpy(dst, "CURRENT|NONE|LOWEST|HIGHEST");
}
