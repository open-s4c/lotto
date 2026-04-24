#include "state.h"
#include <lotto/engine/statemgr.h>
#include <lotto/sys/logger.h>

static qemu_gdb_config_t _config;

REGISTER_CONFIG(_config, {
    logger_infof("enabled = %s\n", _config.enabled ? "on" : "off");
    logger_infof("wait    = %s\n", _config.wait ? "on" : "off");
})

qemu_gdb_config_t *
qemu_gdb_config(void)
{
    return &_config;
}

bool
qemu_gdb_enabled(void)
{
    return _config.enabled;
}

bool
qemu_gdb_get_wait(void)
{
    return _config.wait;
}

void
qemu_gdb_set_wait(bool wait)
{
    _config.wait = wait;
    if (wait) {
        _config.enabled = true;
    }
}

void
qemu_gdb_enable(void)
{
    _config.enabled = true;
}

void
qemu_gdb_disable(void)
{
    _config.enabled = false;
    _config.wait    = false;
}
