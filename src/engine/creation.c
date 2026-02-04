#ifdef DICE_MODULE_SLOT
    #undef DICE_MODULE_SLOT
#endif
#define DICE_MODULE_SLOT 11

#define LOGGER_PREFIX LOGGER_CUR_FILE
#define LOGGER_BLOCK  LOGGER_CUR_BLOCK
#include <lotto/base/tidmap.h>
#include <lotto/base/tidset.h>
#include <lotto/brokers/statemgr.h>
#include <lotto/engine/dispatcher.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/ensure.h>
#include <lotto/sys/logger_block.h>
#include <lotto/sys/string.h>
#include <lotto/util/macros.h>

typedef struct {
    marshable_t m;
    tidset_t registered;
    task_id last_parent;
} state_t;

static state_t _state;

/* preallocated tidset for handling requests */
static tidset_t _tset;

/*******************************************************************************
 * marshaling interface
 ******************************************************************************/
STATIC size_t _creation_size(const marshable_t *m);
STATIC void *_creation_marshal(const marshable_t *m, void *buf);
STATIC const void *_creation_unmarshal(marshable_t *m, const void *buf);

#define MARSHABLE_STATE                                                        \
    ((marshable_t){                                                            \
        .alloc_size = sizeof(state_t),                                         \
        .unmarshal  = _creation_unmarshal,                                     \
        .marshal    = _creation_marshal,                                       \
        .size       = _creation_size,                                          \
    })


/*******************************************************************************
 * init and printing
 ******************************************************************************/
REGISTER_EPHEMERAL(_state, ({
                       _state.last_parent = 1; /* by convention */
                       _state.m           = MARSHABLE_STATE;
                       tidset_init(&_state.registered);
                       tidset_init(&_tset);
                   }))

/*******************************************************************************
 * handler
 ******************************************************************************/
void
handle_creation(const context_t *ctx, event_t *e)
{
    ASSERT(e);
    ASSERT(ctx);
    ASSERT(ctx->id != NO_TASK);
    ASSERT(!e->is_chpt);
    ASSERT(tidset_size(&e->tset) == 0);

    switch (ctx->cat) {
        case CAT_TASK_CREATE:
            _state.last_parent = ctx->id;
            e->next            = ANY_TASK;
            e->is_chpt         = true;
            e->readonly        = true;
            break;

        case CAT_TASK_INIT:
            logger_debugf("Register tid %lu (parent: %lu)\n", ctx->id,
                          _state.last_parent);
            ENSURE(tidset_insert(&_state.registered, ctx->id) &&
                   "a task is reregistered");
            e->reason  = REASON_DETERMINISTIC;
            e->is_chpt = true;
            break;

        case CAT_TASK_FINI:
            ENSURE(tidset_remove(&_state.registered, ctx->id) &&
                   "an unregistered task deregistered");
            e->is_chpt = true;
            break;

        default:
            if (!tidset_insert(&_state.registered, ctx->id))
                break;
            logger_debugf("Registering task %lu\n", ctx->id);
            break;
    }

    /* update the "cache" _tset variable with _state.registered. If the size of
     * _state.registered did not grow much, there should be no reallocation with
     * this copy. It's a simply memcpy of a few uint64_t. */
    tidset_copy(&_tset, &_state.registered);

    /* the following line does a shallow copy of _tset into cp->tset. That
     * avoids any allocation because the array pointer stored in _tset is
     * copied to cp->tset (but not the content of the array). */
    e->tset = _tset;
}

/*******************************************************************************
 * marshaling implementation
 ******************************************************************************/
STATIC size_t
_creation_size(const marshable_t *m)
{
    ASSERT(m);
    state_t *s = (state_t *)m;
    return sizeof(state_t) + marshable_size_m(&s->registered);
}

STATIC void *
_creation_marshal(const marshable_t *m, void *buf)
{
    ASSERT(m);
    ASSERT(buf);

    char *b    = (char *)buf;
    state_t *s = (state_t *)m;

    sys_memcpy(b, s, sizeof(state_t));
    b += sizeof(state_t);

    return marshable_marshal_m(&s->registered, b);
}


STATIC const void *
_creation_unmarshal(marshable_t *m, const void *buf)
{
    ASSERT(m);
    ASSERT(buf);

    size_t off = offsetof(marshable_t, payload);
    sys_memcpy(m->payload, buf + off, m->alloc_size - off);
    char *b = (char *)buf + m->alloc_size;

    state_t *s = (state_t *)m;
    tidset_init(&s->registered);
    return marshable_unmarshal_m(&s->registered, b);
}
