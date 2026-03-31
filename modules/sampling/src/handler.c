#include "state.h"
#include <dice/events/memaccess.h>
#include <dice/events/stacktrace.h>
#include <dice/module.h>
#include <dice/pubsub.h>
#include <lotto/engine/prng.h>
#include <lotto/engine/pubsub.h>
#include <lotto/engine/sequencer.h>
#include <lotto/engine/statemgr.h>
#include <lotto/runtime/capture_point.h>
#include <lotto/runtime/ingress_events.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger.h>
#include <lotto/sys/stdio.h>
#include <lotto/sys/stdlib.h>
#include <lotto/sys/string.h>
#include <lotto/util/macros.h>

/* Amount of sampled events in INTERCEPT_EVENT/INTERCEPT_BEFORE by type.
 *
 * Probability of sampling (ie, not dropping the event) = p, with p \in [0;1].
 * Configuration is stored as d such that
 *
 * * p = 1.0 - d with d \in [0;1].
 *
 * By default nothing is dropped (ie, d = 0 for all events).
 * A configuration file can change p. Note that in the configuration file, we
 * write p, ie, the probability of sampling (not the probability of dropping).
 *
 * For BEFORE/AFTER chains, the sampling decision is carried in Dice metadata.
 * If BEFORE drops an event, INTERCEPT_AFTER drops the matching AFTER event when
 * the publisher reuses the same metadata object for both publications. This
 * works with autocepted events as well. Custom interceptors must follow the
 * same convention if they want sampling to suppress their AFTER event
 * consistently.
 */

static double _drop[MAX_TYPES];

STATIC void _load_state();
LOTTO_SUBSCRIBE(EVENT_ENGINE__AFTER_UNMARSHAL_CONFIG, { _load_state(); })

static type_id
_effective_type(const capture_point *cp)
{
    type_id type = cp->type_id;
    ASSERT(type < MAX_TYPES);
    return type;
}

static void
_print_config()
{
    for (type_id type = 0; type < MAX_TYPES; type++) {
        if (_drop[type] != 0) {
            logger_debugln("%u = %f", type, _drop[type]);
        }
    }
}

static void
_load_state()
{
    bag_t *entries = sampling_entries();

    for (type_id type = 0; type < MAX_TYPES; type++)
        _drop[type] = 0;

    struct sampling_config *cfg = sampling_config();
    for (const bagitem_t *it = bag_iterate(entries); it; it = bag_next(it)) {
        const struct sampling_config_entry *entry =
            (const struct sampling_config_entry *)it;
        type_id type = ps_type_lookup(entry->name);

        if (type >= MAX_TYPES || type == ANY_EVENT) {
            logger_errorf("%s: unknown event type: %s\n", cfg->filename,
                          entry->name);
            continue;
        }

        _drop[type] = 1.0 - entry->p;
    }

    _print_config();
}

PS_SUBSCRIBE(INTERCEPT_EVENT, ANY_EVENT, {
    if (type >= MAX_TYPES) {
        logger_errorf("invalid type id: %d\n", type);
        return PS_OK;
    }
    double p = 1.0 - _drop[type];
    logger_debugf("%s => %f\n", ps_type_str(type), p);

    if (p == 1)
        return PS_OK;
    if (p == 0)
        return PS_STOP_CHAIN;

    double pp = prng_real();
    ASSERT(pp <= 1.0);
    return (pp <= p) ? PS_OK : PS_STOP_CHAIN;
})

PS_SUBSCRIBE(INTERCEPT_BEFORE, ANY_EVENT, {
    ASSERT(md);
    if (type >= MAX_TYPES) {
        logger_errorf("invalid type id: %d\n", type);
        return PS_OK;
    }
    double p = 1.0 - _drop[type];
    logger_debugf("%s => %f\n", ps_type_str(type), p);

    if (p == 1)
        return PS_OK;
    if (p == 0)
        goto drop;

    double pp = prng_real();
    ASSERT(pp <= 1.0);
    if (pp <= p)
        return PS_OK;
drop:
    md->drop = true;
    return PS_STOP_CHAIN;
})

PS_SUBSCRIBE(INTERCEPT_AFTER, ANY_EVENT, {
    ASSERT(md);
    return (md->drop) ? PS_STOP_CHAIN : PS_OK;
})
