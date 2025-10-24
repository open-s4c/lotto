/*
 */
#define LOG_PREFIX LOG_CUR_FILE
#define LOG_BLOCK  LOG_CUR_BLOCK
#include <lotto/brokers/statemgr.h>
#include <lotto/sys/logger_block.h>

typedef struct task_velocity_config {
    marshable_t m;
    bool enabled;
} task_velocity_config_t;
static task_velocity_config_t _config;
REGISTER_CONFIG(_config, {
    log_infof("enabled = %s\n", _config.enabled ? "on" : "off");
})

task_velocity_config_t *
task_velocity_config()
{
    return &_config;
}
