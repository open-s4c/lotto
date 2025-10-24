/*
 */
#define LOG_PREFIX LOG_CUR_FILE
#define LOG_BLOCK  LOG_CUR_BLOCK
#include <lotto/brokers/statemgr.h>
#include <lotto/states/handlers/available.h>
#include <lotto/sys/logger_block.h>
#include <lotto/util/macros.h>

static tidset_t _available_tasks;
STATIC void
_replay_print(const marshable_t *m)
{
    log_infof("available tasks: ");
    tidset_print(m);
}
REGISTER_PERSISTENT(_available_tasks, {
    tidset_init(&_available_tasks);
    _available_tasks.m.print = _replay_print;
})
static available_config_t _config;
REGISTER_CONFIG(_config, {
    log_infof("enabled = %s\n", _config.enabled ? "on" : "off");
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
