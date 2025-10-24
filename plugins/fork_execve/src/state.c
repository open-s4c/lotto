/*
 */
#define LOG_PREFIX LOG_CUR_FILE
#define LOG_BLOCK  LOG_CUR_BLOCK
#include <state.h>

#include <lotto/brokers/statemgr.h>
#include <lotto/sys/logger_block.h>

static fork_execve_config_t _config;
REGISTER_CONFIG(_config, {
    log_infof("enabled = %s\n", _config.enabled ? "on" : "off");
})

fork_execve_config_t *
fork_execve_config()
{
    return &_config;
}
