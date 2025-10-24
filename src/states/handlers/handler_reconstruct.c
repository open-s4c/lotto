/*
 */
#define LOG_PREFIX LOG_CUR_FILE
#define LOG_BLOCK  LOG_CUR_BLOCK
#include <lotto/brokers/pubsub.h>
#include <lotto/brokers/statemgr.h>
#include <lotto/states/handlers/reconstruct.h>
#include <lotto/sys/logger_block.h>
#include <lotto/util/macros.h>

static reconstruct_config_t _config;

REGISTER_CONFIG(_config, {
    log_infoln("enabled: %s", bool_str(_config.enabled));
    log_infoln("tid:     %lu", _config.tid);
    log_infoln("id:      %lu", _config.id);
    log_infoln("data:    0x%lx", _config.data);
})

static log_t _log;
STATIC void
_reconstruct_print(const marshable_t *m)
{
    log_t *l = (log_t *)m;
    if (l->tid == NO_TASK) {
        log_infof("entry: none\n");
        return;
    }
    log_print(m);
}
REGISTER_PERSISTENT(_log, {
    log_init(&_log);
    _log.m.print = _reconstruct_print;
})

log_t *
reconstruct_get_log()
{
    return &_log;
}

reconstruct_config_t *
reconstruct_config()
{
    return &_config;
}
