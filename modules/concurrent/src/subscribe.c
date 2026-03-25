#include <stdbool.h>
#include <stdint.h>

#include "events.h"
#include <dice/module.h>
#include <dice/pubsub.h>
#include <dice/self.h>
#include <lotto/engine/pubsub.h>
#include <lotto/runtime/capture_point.h>
#include <lotto/runtime/ingress_events.h>
#include <lotto/sys/abort.h>
#include <lotto/sys/logger.h>
#include <lotto/unsafe/concurrent.h>

struct concurrent_state {
    bool concurrent;
    uint32_t depth;
    chain_id last_src_chain;
    type_id last_src_type;
    uint64_t dropped;
};

static struct concurrent_state concurrent_;

static inline struct concurrent_state *
concurrent_state_get_(metadata_t *md)
{
    if (md == NULL) {
        return NULL;
    }

    return SELF_TLS(md, &concurrent_);
}

static inline void
concurrent_state_clear_drops_(struct concurrent_state *state)
{
    state->dropped        = 0;
    state->last_src_chain = 0;
    state->last_src_type  = 0;
}

static inline void
concurrent_state_log_leave_(struct concurrent_state *state)
{
    if (state->dropped > 0) {
        logger_debugln(
            "left concurrent region after dropping %lu ingress events; "
            "last=%s/%s",
            state->dropped, ps_chain_str(state->last_src_chain),
            ps_type_str(state->last_src_type));
    }

    concurrent_state_clear_drops_(state);
}

static inline enum ps_err
concurrent_filter_(metadata_t *md, chain_id chain, type_id type)
{
    if (type == EVENT_DETACH || type == EVENT_ATTACH ||
        type == EVENT_TASK_CREATE || type == EVENT_TASK_JOIN ||
        type == EVENT_TASK_FINI) {
        return PS_OK;
    }

    struct concurrent_state *state = concurrent_state_get_(md);
    if (state == NULL || !state->concurrent) {
        return PS_OK;
    }

    if (type == EVENT_TASK_INIT) {
        logger_fatalf("task init seen while task is concurrent");
        sys_abort();
    }

    state->last_src_chain = chain;
    state->last_src_type  = type;
    state->dropped++;
    return PS_STOP_CHAIN;
}

PS_SUBSCRIBE(CHAIN_INGRESS_EVENT, EVENT_DETACH, {
    capture_point *cp              = event;
    struct concurrent_state *state = concurrent_state_get_(md);
    if (state == NULL) {
        return PS_STOP_CHAIN;
    }

    state->depth++;
    if (state->depth == 1) {
        state->concurrent = true;
        cp->blocking      = true;
        PS_PUBLISH(CHAIN_INGRESS_BEFORE, EVENT_DETACH, cp, md);
    }

    return PS_STOP_CHAIN;
})

PS_SUBSCRIBE(CHAIN_INGRESS_EVENT, EVENT_ATTACH, {
    capture_point *cp              = event;
    struct concurrent_state *state = concurrent_state_get_(md);
    if (state == NULL) {
        return PS_STOP_CHAIN;
    }

    if (state->depth == 0) {
        state->concurrent = false;
        concurrent_state_clear_drops_(state);
        return PS_STOP_CHAIN;
    }

    state->depth--;
    if (state->depth > 0) {
        return PS_STOP_CHAIN;
    }

    state->concurrent = false;
    concurrent_state_log_leave_(state);
    cp->blocking = true;
    PS_PUBLISH(CHAIN_INGRESS_AFTER, EVENT_ATTACH, cp, md);
    return PS_STOP_CHAIN;
})

PS_SUBSCRIBE(CHAIN_INGRESS_EVENT, ANY_EVENT,
             { return concurrent_filter_(md, chain, type); })

PS_SUBSCRIBE(CHAIN_INGRESS_BEFORE, ANY_EVENT,
             { return concurrent_filter_(md, chain, type); })

PS_SUBSCRIBE(CHAIN_INGRESS_AFTER, ANY_EVENT,
             { return concurrent_filter_(md, chain, type); })

bool
_lotto_is_concurrent(void)
{
    metadata_t *md                 = self_md();
    struct concurrent_state *state = concurrent_state_get_(md);
    return state != NULL && state->concurrent;
}
