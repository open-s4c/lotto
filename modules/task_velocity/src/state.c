#include <lotto/engine/statemgr.h>
#include <lotto/sys/logger.h>

typedef struct task_velocity_config {
    marshable_t m;
    bool enabled;
} task_velocity_config_t;
static task_velocity_config_t _config;
REGISTER_CONFIG(_config, {
    _config.enabled = true;
    logger_infof("enabled = %s\n", _config.enabled ? "on" : "off");
})

task_velocity_config_t *
task_velocity_config()
{
    return &_config;
}
