#include "state.h"
#include <lotto/base/tidbag.h>
#include <lotto/engine/sequencer.h>
#include <lotto/engine/statemgr.h>
#include <lotto/modules/region_preemption/events.h>
#include <lotto/runtime/capture_point.h>
#include <lotto/sys/logger.h>
#include <lotto/util/macros.h>

static tidbag_t _in_region;

REGISTER_EPHEMERAL(_in_region, { tidbag_init(&_in_region); })

STATIC void
_region_filter_handle(const capture_point *cp, event_t *e)
{
    ASSERT(cp && cp->id != NO_TASK);
    ASSERT(e);

    if (!region_filter_config()->enabled)
        return;

    task_id tid = cp->vid != NO_TASK ? cp->vid : cp->id;

    switch (cp->type_id) {
        case EVENT_REGION_PREEMPTION_IN:
            if (!tidbag_has(&_in_region, tid)) {
                logger_infof("enter region %lx\n", tid);
            }
            tidbag_insert(&_in_region, tid);
            break;
        case EVENT_REGION_PREEMPTION_OUT:
            tidbag_remove(&_in_region, tid);
            if (!tidbag_has(&_in_region, tid)) {
                logger_infof("leave region %lx\n", tid);
            }
            break;
        default:
            break;
    }

    if (tidbag_has(&_in_region, tid)) {
        e->filter_less = true;
    }
}
ON_SEQUENCER_CAPTURE(_region_filter_handle)
