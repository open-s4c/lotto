#define LOGGER_PREFIX LOGGER_CUR_FILE
#define LOGGER_BLOCK  LOGGER_CUR_BLOCK
#include <lotto/brokers/statemgr.h>
#include <lotto/modules/ichpt/state.h>
#include <lotto/sys/logger_block.h>

/* config contains the initial state of the handler */
static ichpt_config_t _config;
static vec_t _initial;

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
    vec_t *v = (vec_t *)m;
    for (size_t i = 0; i < vec_size(v); ++i) {
        _item_print(&vec_get(v, i)->m);
    }
}
REGISTER_STATE(CONFIG, _config, {
    _config.m = MARSHABLE_STATIC(sizeof(ichpt_config_t));
    vec_init(&_initial, MARSHABLE_ITEM);
    _initial.m.print = _print_m;
    marshable_bind(&_config.m, &_initial.m);
})

static vec_t _final;
REGISTER_STATE(FINAL, _final, {
    vec_init(&_final, MARSHABLE_ITEM);
    _final.m.print = _print_m;
})

ichpt_config_t *
ichpt_config()
{
    return &_config;
}

vec_t *
ichpt_initial()
{
    return &_initial;
}

vec_t *
ichpt_final()
{
    return &_final;
}

int
ichpt_item_compare(const vecitem_t *ia, const vecitem_t *ib)
{
    const item_t *a = (const item_t *)ia;
    const item_t *b = (const item_t *)ib;
    return stable_address_compare(&a->addr, &b->addr);
}
