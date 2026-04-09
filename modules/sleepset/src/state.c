#include "state.h"
#include <lotto/engine/statemgr.h>
#include <lotto/sys/logger.h>

static sleepset_config_t _config;
static sleepset_state_t _state;
static bool _state_initd;

REGISTER_CONFIG(_config, {
    logger_infof("enabled = %s\n", _config.enabled ? "on" : "off");
})

void
sleepset_reset(void)
{
    if (!_state_initd) {
        tidset_init(&_state.sleeping);
        tidset_init(&_state.prev_enabled);
        tidset_init(&_state.cur_enabled);
        tidset_init(&_state.next_sleeping);
        tidmap_init(&_state.accesses,
                    MARSHABLE_STATIC(sizeof(sleepset_task_access_t)));
        _state_initd = true;
    }

    tidset_clear(&_state.sleeping);
    tidset_clear(&_state.prev_enabled);
    tidset_clear(&_state.cur_enabled);
    tidset_clear(&_state.next_sleeping);
    tidmap_clear(&_state.accesses);
}
REGISTER_EPHEMERAL(_state, { sleepset_reset(); })

sleepset_config_t *
sleepset_config()
{
    return &_config;
}

sleepset_state_t *
sleepset_state()
{
    return &_state;
}
