/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2025. All rights reserved.
 * SPDX-License-Identifier: 0BSD
 */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#define DICE_MODULE_PRIO 1
#include <dice/mempool.h>
#include <dice/module.h>
#include <dice/pubsub.h>

#define MAX_SUBSCRIPTIONS 255

int ps_dispatch_max(void);
bool ps_dispatch_chain_on_(chain_id);

struct sub {
    chain_id chain;
    chain_id type;
    ps_callback_f cb;
    int prio;
    struct sub *next;
};

struct type {
    size_t count;
    struct sub *head;
};

struct chain {
    struct type types[MAX_TYPES];
};

static struct chain _chains[MAX_CHAINS];

// -----------------------------------------------------------------------------
// initializer
// -----------------------------------------------------------------------------

DICE_HIDE bool
ps_initd_(void)
{
    static enum {
        NONE,
        START,
        BLOCK,
    } _state          = NONE;
    static bool ready = false;

    if (likely(ready)) {
        return true;
    }

    switch (_state) {
        case NONE:
            // This must be the main thread, at latest the thread creation.
            _state = START;
            PS_PUBLISH(CHAIN_CONTROL, EVENT_DICE_INIT, 0, 0);
            ready = true;
            PS_PUBLISH(CHAIN_CONTROL, EVENT_DICE_READY, 0, 0);
            return true;
        case START:
            _state = BLOCK;
            return true;
        case BLOCK:
            // The publication above is still running, drop nested publications.
            return false;
        default:
            __builtin_unreachable();
            return true;
    }
}


DICE_HIDE void
ps_init_(void)
{
    (void)ps_initd_();
}

DICE_MODULE_INIT()

// -----------------------------------------------------------------------------
// subscribe interface
// -----------------------------------------------------------------------------

static void
_ps_subscribe_sorted(struct sub **cur, chain_id chain, type_id type,
                     ps_callback_f cb, int prio, bool any_type)
{
    // any_type is set if this subscription was from ANY_TYPE
    struct sub *sub;
    struct sub *next = NULL;

    if (*cur == NULL || prio < (*cur)->prio) {
        next = *cur;
        goto insert;
    } else if (prio == (*cur)->prio && !any_type) {
        next = *cur;
        goto insert;
    } else {
        _ps_subscribe_sorted(&(*cur)->next, chain, type, cb, prio, any_type);
        return;
    }

insert:
    // create and add
    sub  = (struct sub *)mempool_alloc(sizeof(struct sub));
    *sub = (struct sub){.chain = chain,
                        .type  = type,
                        .cb    = cb,
                        .prio  = prio,
                        .next  = next};
    *cur = sub;
}

static int
_ps_subscribe_type(chain_id chain, type_id type, ps_callback_f cb, int prio,
                   bool any_type)
{
    (void)prio;
    if (type > MAX_TYPES)
        return PS_INVALID;

    struct type *ev = &_chains[chain].types[type];

    // increment the idx of next subscription
    ev->count++;

    // register subscription
    _ps_subscribe_sorted(&ev->head, chain, type, cb, prio, any_type);

    return PS_OK;
}

int
ps_subscribe(chain_id chain, type_id type, ps_callback_f cb, int prio)
{
    ps_init_(); // ensure initialized

    log_debug("Subscribe %u_%u_%d", chain, type, prio);
    if (ps_dispatch_chain_on_(chain) && prio <= ps_dispatch_max()) {
        log_debug("Ignore subscription %u_%u_%d", chain, type, prio);
        return PS_OK;
    }
    if (chain == CHAIN_CONTROL)
        return PS_OK;
    if (chain > MAX_CHAINS)
        return PS_INVALID;
    if (type != ANY_TYPE)
        return _ps_subscribe_type(chain, type, cb, prio, false);

    int err;
    for (size_t i = 1; i < MAX_TYPES; i++)
        if ((err = _ps_subscribe_type(chain, i, cb, prio, true)) != 0)
            return err;

    return PS_OK;
}

// -----------------------------------------------------------------------------
// publish interface
// -----------------------------------------------------------------------------

static enum ps_err
_ps_publish(const chain_id chain, const type_id type, void *event,
            metadata_t *md)
{
    if (unlikely(chain >= MAX_CHAINS))
        return PS_INVALID;
    if (unlikely(type == ANY_TYPE || type >= MAX_TYPES))
        return PS_INVALID;

    struct type *ev = &_chains[chain].types[type];
    struct sub *cur = ev->head;
    while (cur) {
        // now we call the callback and abort the chain if the subscriber
        // "censors" the type by returning PS_STOP_CHAIN.
        enum ps_err err = cur->cb(chain, type, event, md);
        if (err == PS_STOP_CHAIN)
            break;
        if (err == PS_DROP_EVENT)
            return PS_DROP_EVENT;
        cur = cur->next;
    }
    return PS_OK;
}

DICE_WEAK DICE_HIDE enum ps_err
ps_dispatch_(const chain_id chain, const type_id type, void *event,
             metadata_t *md)
{
    (void)chain;
    (void)type;
    (void)event;
    (void)md;
    return PS_HANDLER_OFF;
}

DICE_WEAK enum ps_err
ps_publish(const chain_id chain, const type_id type, void *event,
           metadata_t *md)
{
    if (unlikely(!ps_initd_()))
        return PS_DROP_EVENT;

    log_debug("Publish %u_%u", chain, type);
    enum ps_err err = ps_dispatch_(chain, type, event, md);

    if (likely(err == PS_STOP_CHAIN))
        return PS_OK;

    if (likely(err == PS_DROP_EVENT))
        return PS_DROP_EVENT;

    return _ps_publish(chain, type, event, md);
}
