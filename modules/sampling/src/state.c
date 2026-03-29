#include "state.h"
#include <lotto/engine/pubsub.h>
#include <lotto/engine/statemgr.h>
#include <lotto/sys/logger.h>

static struct sampling_config _config;
static bag_t _entries;

static void
_entry_print(const marshable_t *m)
{
    const struct sampling_config_entry *entry =
        (const struct sampling_config_entry *)m;
    logger_infof("%s = %f\n", entry->name, entry->p);
}

static void
_config_print(const marshable_t *m)
{
    (void)m;
    logger_infof("filename = %s\n", _config.filename);
    sampling_config_print();
}

REGISTER_STATE(CONFIG, _config, {
    _config.m       = MARSHABLE_STATIC(sizeof(struct sampling_config));
    _config.m.print = _config_print;
    bag_init(&_entries,
             MARSHABLE_STATIC_PRINTABLE(sizeof(struct sampling_config_entry),
                                        _entry_print));
    marshable_bind(&_config.m, &_entries.m);
})

struct sampling_config *
sampling_config()
{
    return &_config;
}

bag_t *
sampling_entries(void)
{
    return &_entries;
}

void
sampling_config_print(void)
{
    const bagitem_t *it = NULL;

    for (it = bag_iterate(sampling_entries()); it != NULL; it = bag_next(it))
        _entry_print((const marshable_t *)it);
}
