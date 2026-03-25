#include <stdbool.h>
#include <stdint.h>

#include "events.h"
#include <dice/chains/capture.h>
#include <dice/module.h>
#include <dice/pubsub.h>
#include <dice/self.h>
#include <lotto/engine/pubsub.h>
#include <lotto/runtime/capture_point.h>
#include <lotto/runtime/ingress_events.h>
#include <lotto/sys/abort.h>
#include <lotto/sys/logger.h>
#include <lotto/unsafe/ghost.h>

struct ghost_state {
    bool ghosted;
    uint32_t depth;
    chain_id last_src_chain;
    type_id last_src_type;
    uint64_t dropped;
};

static struct ghost_state ghost_state_;

static inline struct ghost_state *
ghost_state_get_(metadata_t *md)
{
    if (md == NULL) {
        return NULL;
    }

    return SELF_TLS(md, &ghost_state_);
}

static inline void
ghost_state_clear_drops_(struct ghost_state *state)
{
    state->dropped        = 0;
    state->last_src_chain = 0;
    state->last_src_type  = 0;
}

static inline void
ghost_state_log_leave_(struct ghost_state *state)
{
    if (state->dropped > 0) {
        logger_debugln(
            "left ghost region after dropping %lu ingress events; last=%s/%s",
            state->dropped, ps_chain_str(state->last_src_chain),
            ps_type_str(state->last_src_type));
    }

    ghost_state_clear_drops_(state);
}

static inline enum ps_err
ghost_filter_(metadata_t *md, chain_id chain, type_id type)
{
    if (type == EVENT_GHOST_START || type == EVENT_GHOST_END ||
        type == EVENT_TASK_CREATE || type == EVENT_TASK_JOIN ||
        type == EVENT_TASK_FINI) {
        return PS_OK;
    }

    struct ghost_state *state = ghost_state_get_(md);
    if (state == NULL || !state->ghosted) {
        return PS_OK;
    }

    if (type == EVENT_TASK_INIT) {
        logger_fatalf("task init seen while task is ghosted");
        sys_abort();
    }

    state->last_src_chain = chain;
    state->last_src_type  = type;
    state->dropped++;
    return PS_STOP_CHAIN;
}

LOTTO_ADVERTISE_TYPE(EVENT_GHOST_START)
LOTTO_ADVERTISE_TYPE(EVENT_GHOST_END)

PS_SUBSCRIBE(CAPTURE_EVENT, EVENT_GHOST_START, {
    capture_point cp = {
        .src_chain = chain,
        .src_type  = EVENT_GHOST_START,
        .func      = "lotto_ghost_enter",
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_GHOST_START, &cp, md);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_EVENT, EVENT_GHOST_END, {
    capture_point cp = {
        .src_chain = chain,
        .src_type  = EVENT_GHOST_END,
        .func      = "lotto_ghost_leave",
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_GHOST_END, &cp, md);
    return PS_OK;
})

PS_SUBSCRIBE(CHAIN_INGRESS_EVENT, EVENT_GHOST_START, {
    struct ghost_state *state = ghost_state_get_(md);
    if (state != NULL) {
        state->depth++;
        state->ghosted = true;
    }

    return PS_STOP_CHAIN;
})

PS_SUBSCRIBE(CHAIN_INGRESS_EVENT, EVENT_GHOST_END, {
    struct ghost_state *state = ghost_state_get_(md);
    if (state == NULL) {
        return PS_STOP_CHAIN;
    }

    if (state->depth == 0) {
        state->ghosted = false;
        ghost_state_clear_drops_(state);
        return PS_STOP_CHAIN;
    }

    state->depth--;
    if (state->depth > 0) {
        return PS_STOP_CHAIN;
    }

    state->ghosted = false;
    ghost_state_log_leave_(state);
    return PS_STOP_CHAIN;
})

PS_SUBSCRIBE(CHAIN_INGRESS_EVENT, ANY_EVENT,
             { return ghost_filter_(md, chain, type); })

PS_SUBSCRIBE(CHAIN_INGRESS_BEFORE, ANY_EVENT,
             { return ghost_filter_(md, chain, type); })

PS_SUBSCRIBE(CHAIN_INGRESS_AFTER, ANY_EVENT,
             { return ghost_filter_(md, chain, type); })

bool
_lotto_ghosted()
{
    metadata_t *md            = self_md();
    struct ghost_state *state = ghost_state_get_(md);
    return state != NULL && state->ghosted;
}
