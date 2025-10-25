#include <lotto/states/handlers/deadlock.h>
#include <lotto/states/handlers/mutex.h>

static deadlock_config_t g_deadlock_config = {
    .m = {0},
    .enabled = false,
    .lost_resource_check = false,
    .extra_release_check = false,
};

deadlock_config_t *
deadlock_config(void)
{
    return &g_deadlock_config;
}

static mutex_config_t g_mutex_config = {
    .m = {0},
    .enabled = false,
    .deadlock_check = false,
};

mutex_config_t *
mutex_config(void)
{
    return &g_mutex_config;
}
