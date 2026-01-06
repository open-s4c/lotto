#include <lotto/states/handlers/deadlock.h>
#include <lotto/states/handlers/mutex.h>

static mutex_config_t g_mutex_config = {
    .m              = {0},
    .enabled        = false,
    .deadlock_check = false,
};

mutex_config_t *
mutex_config(void)
{
    return &g_mutex_config;
}
