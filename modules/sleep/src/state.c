#include "state.h"
#include <lotto/engine/statemgr.h>
#include <lotto/sys/logger.h>
#include <lotto/sys/stdio.h>
#include <lotto/sys/stdlib.h>
#include <lotto/sys/string.h>

static sleep_config_t _config = {.mode = SLEEP_MODE_UNTIL};

REGISTER_CONFIG(_config,
                { logger_infof("mode = %s\n", sleep_mode_str(_config.mode)); })

const char *
sleep_mode_str(uint64_t mode)
{
    switch (mode) {
        case SLEEP_MODE_ONCE:
            return "once";
        case SLEEP_MODE_UNTIL:
            return "until";
    }
    return "until";
}

uint64_t
sleep_mode_from(const char *mode)
{
    if (sys_strcmp(mode, "once") == 0) {
        return SLEEP_MODE_ONCE;
    }
    if (sys_strcmp(mode, "until") == 0) {
        return SLEEP_MODE_UNTIL;
    }
    sys_fprintf(stderr, "error: invalid sleep mode '%s'; expected once|until\n",
                mode);
    sys_exit(1);
}

void
sleep_mode_all_str(char *str)
{
    sys_strcpy(str, "once|until");
}

sleep_config_t *
sleep_config(void)
{
    return &_config;
}
