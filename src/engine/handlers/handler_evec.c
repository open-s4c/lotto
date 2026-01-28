/*
 */

#define LOGGER_PREFIX LOGGER_CUR_FILE
#define LOGGER_BLOCK  LOGGER_CUR_BLOCK
#include <lotto/base/map.h>
#include <lotto/brokers/pubsub.h>
#include <lotto/brokers/pubsub_interface.h>
#include <lotto/brokers/statemgr.h>
#include <lotto/engine/dispatcher.h>
#include <lotto/engine/handlers/timeout.h>
#include <lotto/engine/prng.h>
#include <lotto/evec.h>
#include <lotto/sys/logger_block.h>
#include <lotto/util/casts.h>
#include <lotto/util/macros.h>

typedef struct {
    mapitem_t ti;
    struct timespec deadline;
    enum lotto_timed_wait_status *ret;
} deadline_t;

#define DEADLINE_ELEMENT MARSHABLE_STATIC(sizeof(deadline_t))

struct evec {
    mapitem_t ti;
    tidset_t waiters;
    map_t deadlines;
    int count;
};

struct handler_evec {
    map_t evecs;
    tidset_t waiters;
    struct timespec now;
    bool received_timeout;
};

static struct handler_evec _state;

REGISTER_STATE(EPHEMERAL, _state, {
    map_init(&_state.evecs, MARSHABLE_STATIC(sizeof(struct evec)));
    tidset_init(&_state.waiters);
})

static void _check_timeouts(bool should_publish);
PS_SUBSCRIBE_INTERFACE(TOPIC_TRIGGER_TIMEOUT, {
    if (_state.received_timeout || !tidset_has(&_state.waiters, as_uval(v))) {
        return PS_OK;
    }
    _state.received_timeout = true;
    clock_time(&_state.now);
    _check_timeouts(false);
})

static struct evec *
_evec_find(uint64_t eid)
{
    return (struct evec *)map_find(&_state.evecs, eid);
}

static struct evec *
_evec_init(uint64_t eid)
{
    struct evec *evec = _evec_find(eid);
    if (evec == NULL) {
        evec = (struct evec *)map_register(&_state.evecs, eid);
        ASSERT(evec);
        tidset_init(&evec->waiters);
        map_init(&evec->deadlines, DEADLINE_ELEMENT);
    }
    return evec;
}

static void
_evec_deinit(struct evec *evec)
{
    tidset_fini(&evec->waiters);
    map_clear(&evec->deadlines);
    map_deregister(&_state.evecs, evec->ti.key);
}

static void
_posthandle_prepare(task_id id, uint64_t eid)
{
    logger_debugf("[%lu] evec prepare 0x%lx\n", id, eid);
    ASSERT(!tidset_has(&_state.waiters, id));

    struct evec *evec = _evec_init(eid);
    tidset_insert(&evec->waiters, id);
}

static void
_posthandle_cancel(task_id id, uint64_t eid)
{
    logger_debugf("[%lu] evec cancel 0x%lx\n", id, eid);

    struct evec *evec = _evec_find(eid);
    if (evec == NULL)
        return;

    ASSERT(!tidset_has(&_state.waiters, id));
    tidset_remove(&evec->waiters, id);

    if (tidset_size(&evec->waiters) == 0) {
        // TODO: proper clean up
        // map_deregister(&_state.evecphores, sid);
    }
}

static bool
_handle_wait(task_id id, uint64_t eid)
{
    logger_debugf("[%lu] evec wait 0x%lx\n", id, eid);

    struct evec *evec = _evec_find(eid);
    if (evec == NULL)
        return false;

    ASSERT(!tidset_has(&_state.waiters, id));

    if (!tidset_has(&evec->waiters, id))
        return false;

    tidset_insert(&_state.waiters, id);
    return true;
}

static bool
_handle_timed_wait(task_id id, uint64_t eid,
                   const struct timespec *restrict abstime,
                   enum lotto_timed_wait_status *ret)
{
    logger_debugf("[%lu] evec timed wait 0x%lx\n", id, eid);

    struct evec *evec = _evec_find(eid);
    if (evec == NULL)
        return false;

    ASSERT(!tidset_has(&_state.waiters, id));

    if (!tidset_has(&evec->waiters, id))
        return false;

    deadline_t *deadline = (deadline_t *)map_register(&evec->deadlines, id);
    ASSERT(ret);
    deadline->ret = ret;
    if (!abstime) {
        *ret = TIMED_WAIT_INVALID;
        handler_timeout_deregister(id);
        map_deregister(&evec->deadlines, id);
        return false;
    }
    // TODO other validity checks on abstime
    // deadline->deadline = *abstime;
    deadline->deadline = _state.now;
    deadline->deadline.tv_sec += 20;
    tidset_insert(&_state.waiters, id);
    handler_timeout_register(id, &deadline->deadline);
    return true;
}

static void
_posthandle_wake(task_id id, uint64_t eid, uint32_t cnt)
{
    logger_debugf("[%lu] evec wake %u waiters 0x%lx \n", id, cnt, eid);

    struct evec *evec = _evec_find(eid);
    if (evec == NULL)
        return;

    while (cnt-- > 0 && tidset_size(&evec->waiters) > 0) {
        task_id next = tidset_get(&evec->waiters,
                                  prng_range(0, tidset_size(&evec->waiters)));
        ASSERT(next != NO_TASK);
        tidset_remove(&evec->waiters, next);
        deadline_t *d = (deadline_t *)map_find(&evec->deadlines, next);
        if (d) {
            *(d->ret) = TIMED_WAIT_INVALID;
            handler_timeout_deregister(next);
            map_deregister(&evec->deadlines, next);
        }
        tidset_remove(&_state.waiters, next);
    }

    if (tidset_size(&evec->waiters) == 0) {
        // TODO: proper clean up
        // map_deregister(&_state.evecphores, sid);
    }
}

static void
_posthandle_move(task_id id, uint64_t src, uint64_t dst)
{
    logger_debugf("[%lu] evec move waiters from 0x%lx to 0x%lx \n", id, src,
                  dst);

    ASSERT(!tidset_has(&_state.waiters, id));

    struct evec *esrc = _evec_find(src);
    if (esrc == NULL)
        return;

    if (tidset_size(&esrc->waiters) == 0)
        return;

    struct evec *edst = _evec_find(dst);
    if (edst == NULL)
        edst = _evec_init(dst);

    /* move waiters to dst */
    tidset_union(&edst->waiters, &esrc->waiters);
    tidset_clear(&esrc->waiters);
    ASSERT(!tidset_has(&edst->waiters, id));
    ASSERT(!tidset_has(&esrc->waiters, id));

    /* move deadlines to dst */
    const mapitem_t *it = map_iterate(&esrc->deadlines);
    for (; it; it = map_next(it)) {
        deadline_t *s = (deadline_t *)it;
        deadline_t *d = (deadline_t *)map_register(&edst->deadlines, it->key);
        d->deadline   = s->deadline;
        d->ret        = s->ret;
        handler_timeout_register(it->key, &d->deadline);
    }
    map_clear(&esrc->deadlines);

    if (tidset_size(&esrc->waiters) == 0) {
        _evec_deinit(esrc);
    }
}

static bool
_deadline_met(const mapitem_t *item)
{
    deadline_t *d = (deadline_t *)item;
    if (timespec_compare(&d->deadline, &_state.now) <= 0)
        return true;
    return false;
}

static void
_check_timeouts(bool should_publish)
{
    bool found = false;
    for (const mapitem_t *i = map_iterate(&_state.evecs); i; i = map_next(i)) {
        struct evec *e = (struct evec *)i;
        for (mapitem_t *ii = map_find_first(&e->deadlines, _deadline_met); ii;
             ii            = map_find_first(&e->deadlines, _deadline_met)) {
            found      = true;
            task_id id = ii->key;
            ASSERT(tidset_has(&_state.waiters, id));
            tidset_remove(&_state.waiters, id);
            deadline_t *d = (deadline_t *)ii;
            (*d->ret)     = TIMED_WAIT_TIMEOUT;
            map_deregister(&e->deadlines, id);
            if (should_publish) {
                handler_timeout_deregister(id);
            }
        }
    }
    ASSERT(should_publish || found);
}

static bool
_should_wait(task_id id)
{
    return tidset_has(&_state.waiters, id);
}

static void
_evec_handle(const context_t *ctx, event_t *e)
{
    ASSERT(e);
    ASSERT(ctx);
    ASSERT(ctx->id != NO_TASK);

    uint64_t eid   = ctx->args[0].value.u64;
    bool may_block = false;
    clock_time(&_state.now);
    _state.received_timeout = false;
    _check_timeouts(true);
    switch (ctx->cat) {
        case CAT_EVEC_WAIT:
            _handle_wait(ctx->id, eid);
            may_block = e->is_chpt = true;
            break;
        case CAT_EVEC_TIMED_WAIT:
            _handle_timed_wait(
                ctx->id, eid, (const struct timespec *)ctx->args[1].value.ptr,
                (enum lotto_timed_wait_status *)ctx->args[2].value.ptr);
            may_block = e->is_chpt = true;
            break;
        case CAT_EVEC_PREPARE:
        case CAT_EVEC_CANCEL:
        case CAT_EVEC_WAKE:
        case CAT_EVEC_MOVE:
            e->is_chpt = true;
            break;
        default:
            break;
    }

    /* remove waiting tasks from the tset */
    tidset_subtract(&e->tset, &_state.waiters);

    if (may_block) {
        cappt_add_any_task_filter(e, _should_wait);
    }
}
REGISTER_HANDLER(SLOT_EVEC, _evec_handle);

PS_SUBSCRIBE_INTERFACE(TOPIC_NEXT_TASK, {
    const context_t *ctx = (context_t *)as_any(v);
    ASSERT(ctx);

    uint64_t eid = ctx->args[0].value.u64;
    switch (ctx->cat) {
        case CAT_EVEC_PREPARE:
            _posthandle_prepare(ctx->id, eid);
            break;
        case CAT_EVEC_CANCEL:
            _posthandle_cancel(ctx->id, eid);
            break;
        case CAT_EVEC_WAKE:
            _posthandle_wake(ctx->id, eid, ctx->args[1].value.u32);
            break;
        case CAT_EVEC_MOVE:
            _posthandle_move(ctx->id, eid, ctx->args[1].value.u64);
            break;
        default:
            break;
    }
})

struct handler_evec *
_lotto_evec_handler()
{
    return &_state;
}

bool
_lotto_evec_is_waiter_of(task_id id, void *addr)
{
    struct evec *evec = (struct evec *)map_find(
        &_state.evecs, CAST_TYPE(uint64_t, (uintptr_t)addr));
    if (evec == NULL)
        return false;
    return tidset_has(&evec->waiters, id);
}

bool
_lotto_evec_is_waiting(task_id id)
{
    return tidset_has(&_state.waiters, id);
}


void
_lotto_print_handle_waiters(void)
{
    tidset_print(&_state.waiters.m);
}
