/*
 */
#define LOGGER_PREFIX LOGGER_CUR_FILE
#define LOGGER_BLOCK  LOGGER_CUR_BLOCK
#include <state.h>

#include <lotto/brokers/statemgr.h>
#include <lotto/sys/logger_block.h>
#include <lotto/sys/string.h>

static qemu_config_t _config;
REGISTER_STATE(EPHEMERAL, _config, {
    char *lotto_gdb_enabled_s = getenv("LOTTO_QEMU_GDB");
    if (lotto_gdb_enabled_s) {
        ASSERT(
            sys_strlen(lotto_gdb_enabled_s) == 1 &&
            (lotto_gdb_enabled_s[0] == '0' || lotto_gdb_enabled_s[0] == '1'));
        _config.gdb_enabled = lotto_gdb_enabled_s[0] == '1';
    }
    char *lotto_gdb_wait_s = getenv("LOTTO_QEMU_GDB_WAIT");
    if (lotto_gdb_wait_s) {
        ASSERT(sys_strlen(lotto_gdb_wait_s) == 1 &&
               (lotto_gdb_wait_s[0] == '0' || lotto_gdb_wait_s[0] == '1'));
        _config.gdb_wait = lotto_gdb_wait_s[0] == '1';
        if (_config.gdb_wait) {
            _config.gdb_enabled = true;
        }
    }
})

qemu_config_t *
qemu_config()
{
    return &_config;
}

bool
qemu_gdb_enabled(void)
{
    return _config.gdb_enabled;
}

bool
qemu_gdb_get_wait(void)
{
    return _config.gdb_wait;
}

void
qemu_gdb_set_wait(bool wait)
{
    _config.gdb_wait = wait;
}

void
qemu_gdb_enable(void)
{
    _config.gdb_enabled = true;
}

void
qemu_gdb_disable(void)
{
    _config.gdb_enabled = false;
}
