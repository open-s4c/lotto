#include "state.h"
#include <lotto/base/tidset.h>
#include <lotto/engine/sequencer.h>
#include <lotto/modules/bias/events.h>
#include <lotto/runtime/capture_point.h>
#include <lotto/sys/assert.h>

static int
_cmp_lowest(const void *a, const void *b)
{
    const task_id *ta = (const task_id *)a;
    const task_id *tb = (const task_id *)b;
    return *ta > *tb ? 1 : *ta < *tb ? -1 : 0;
}

static int
_cmp_highest(const void *a, const void *b)
{
    return _cmp_lowest(b, a);
}

static void
_bias_apply(const capture_point *cp, event_t *e)
{
    switch (bias_state()->current_policy) {
        case BIAS_POLICY_NONE:
            return;
        case BIAS_POLICY_CURRENT:
            if (!tidset_has(&e->tset, cp->id))
                return;
            tidset_make_first(&e->tset, cp->id);
            break;
        case BIAS_POLICY_LOWEST:
            tidset_sort(&e->tset, _cmp_lowest);
            break;
        case BIAS_POLICY_HIGHEST:
            tidset_sort(&e->tset, _cmp_highest);
            break;
        default:
            ASSERT(false && "unexpected bias policy");
            return;
    }

    e->selector = SELECTOR_FIRST;
    e->readonly = true;
}

STATIC void
_bias_handle(const capture_point *cp, event_t *e)
{
    ASSERT(cp);
    ASSERT(e);

    switch (cp->type_id) {
        case EVENT_BIAS_POLICY:
            bias_state()->current_policy =
                ((bias_policy_event_t *)cp->payload)->policy;
            break;
        case EVENT_BIAS_TOGGLE: {
            bias_policy_t tmp            = bias_state()->current_policy;
            bias_state()->current_policy = bias_state()->toggle_policy;
            bias_state()->toggle_policy  = tmp;
            break;
        }
        default:
            break;
    }

    if (e->skip || e->readonly || e->selector != SELECTOR_UNDEFINED ||
        tidset_size(&e->tset) <= 1) {
        return;
    }

    _bias_apply(cp, e);
}
ON_SEQUENCER_CAPTURE(_bias_handle)
