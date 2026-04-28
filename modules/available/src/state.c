#include <lotto/engine/statemgr.h>
#include <lotto/modules/available/state.h>
#include <lotto/sys/logger.h>
#include <lotto/util/macros.h>

static tidset_t _available_tasks;
STATIC void
_replay_print(const marshable_t *m)
{
    logger_infof("available tasks: ");
    tidset_print(m);
}
REGISTER_PERSISTENT(_available_tasks, {
    tidset_init(&_available_tasks);
    _available_tasks.m.print = _replay_print;
})
static available_config_t _config = {.enabled = true};
REGISTER_CONFIG(_config, {
    _config.enabled = true;
    logger_infof("enabled = %s\n", _config.enabled ? "on" : "off");
})

tidset_t *
get_available_tasks()
{
    return &_available_tasks;
}

available_config_t *
available_config()
{
    return &_config;
}
