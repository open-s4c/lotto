/*
 */
#define LOGGER_PREFIX LOGGER_CUR_FILE
#define LOGGER_BLOCK  LOGGER_CUR_BLOCK
#include <lotto/brokers/statemgr.h>
#include <lotto/states/handlers/pct.h>
#include <lotto/sys/logger_block.h>
#include <lotto/sys/string.h>
#include <lotto/util/macros.h>

/*******************************************************************************
 * config
 ******************************************************************************/
static pct_config_t _config;
REGISTER_CONFIG(_config, {
    logger_infof("params = [k: %lu, d: %lu, on:%s]\n", _config.k, _config.d,
              _config.enabled ? "on" : "off");
})

pct_config_t *
pct_config()
{
    return &_config;
}

/*******************************************************************************
 * state
 ******************************************************************************/
#define MARSHABLE_TASK MARSHABLE_STATIC(sizeof(task_t))

static pct_state_t _state;

#define MARSHABLE_STATE                                                        \
    ((marshable_t){                                                            \
        .alloc_size = sizeof(pct_state_t),                                     \
        .unmarshal  = _pct_unmarshal,                                          \
        .marshal    = _pct_marshal,                                            \
        .size       = _pct_size,                                               \
        .print      = _pct_print,                                              \
    })

STATIC size_t _pct_size(const marshable_t *m);
STATIC void *_pct_marshal(const marshable_t *m, void *buf);
STATIC const void *_pct_unmarshal(marshable_t *m, const void *buf);
STATIC void _pct_print(const marshable_t *m);

REGISTER_STATE(FINAL, _state, {
    _state.m = MARSHABLE_STATE;
    tidmap_init(&_state.map, MARSHABLE_TASK);
})

pct_state_t *
pct_state()
{
    return &_state;
}

/*******************************************************************************
 * marshaling implementation
 ******************************************************************************/
STATIC size_t
_pct_size(const marshable_t *m)
{
    ASSERT(m);
    return sizeof(pct_state_t);
}

STATIC void *
_pct_marshal(const marshable_t *m, void *buf)
{
    ASSERT(m);
    ASSERT(buf);

    char *b        = (char *)buf;
    pct_state_t *s = (pct_state_t *)m;
    sys_memcpy(b, s, sizeof(pct_state_t));
    b += sizeof(pct_state_t);
    return b;
}

STATIC const void *
_pct_unmarshal(marshable_t *m, const void *buf)
{
    ASSERT(m);
    ASSERT(buf);

    size_t off     = offsetof(pct_state_t, payload);
    pct_state_t *s = (pct_state_t *)m;
    sys_memcpy(s->payload, buf + off, m->alloc_size - off);
    char *b = (char *)buf + m->alloc_size;
    return b;
}

STATIC void
_pct_print(const marshable_t *m)
{
    const pct_state_t *s = (const pct_state_t *)m;
    logger_infof("counts = [kmax: %lu, nmax: %lu, chpts: %lu]\n", s->counts.kmax,
              s->counts.nmax, s->counts.chpts);
}
