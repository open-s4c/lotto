/*
 */
#define LOG_PREFIX LOG_CUR_FILE
#define LOG_BLOCK  LOG_CUR_BLOCK
#include <lotto/brokers/statemgr.h>
#include <lotto/states/handlers/ichpt.h>
#include <lotto/sys/logger_block.h>

/* config contains the initial state of the handler */
static ichpt_config_t _config;
static bag_t _initial;

static void
_item_print(const marshable_t *m)
{
    char str[STABLE_ADDRESS_MAX_STR_LEN];
    const item_t *i = (item_t *)m;
    stable_address_sprint(&i->addr, str);
    logger_infof("  %s\n", str);
}
#define MARSHABLE_ITEM MARSHABLE_STATIC_PRINTABLE(sizeof(item_t), _item_print)

static void
_print_m(const marshable_t *m)
{
    logger_infof("instruction addresses:\n");
    bag_t *b = (bag_t *)m;
    const bagitem_t *it;
    for (it = bag_iterate(b); it; it = bag_next(it)) {
        _item_print(&it->m);
    }
}
REGISTER_STATE(CONFIG, _config, {
    _config.m = MARSHABLE_STATIC(sizeof(ichpt_config_t));
    bag_init(&_initial, MARSHABLE_ITEM);
    _initial.m.print = _print_m;
    marshable_bind(&_config.m, &_initial.m);
})

static bag_t _final;
REGISTER_STATE(FINAL, _final, {
    bag_init(&_final, MARSHABLE_ITEM);
    _final.m.print = _print_m;
})

ichpt_config_t *
ichpt_config()
{
    return &_config;
}

bag_t *
ichpt_initial()
{
    return &_initial;
}

bag_t *
ichpt_final()
{
    return &_final;
}
