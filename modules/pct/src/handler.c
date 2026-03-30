/*******************************************************************************
 * Implements PCT
 *
 * https://www.microsoft.com/en-us/research/wp-content/uploads/2016/02/asplos277-pct.pdf
 ******************************************************************************/

#include <math.h>

#include "state.h"
#include <lotto/engine/prng.h>
#include <lotto/engine/sequencer.h>
#include <lotto/engine/state.h>
#include <lotto/engine/statemgr.h>
#include <lotto/runtime/ingress_events.h>
#include <lotto/sys/logger.h>
#include <lotto/util/macros.h>
#include <lotto/util/once.h>

/*******************************************************************************
 * internal functions
 ******************************************************************************/
static uint64_t
_pct_priority(uint64_t rval)
{
    return rval;
}

static bool
_pct_is_change_point(uint64_t rand_val)
{
    return pct_state()->counts.chpts < pct_config()->chpts &&
           (rand_val % pct_config()->k) <= pct_config()->d;
}

static void
_pct_update(task_t *t, uint64_t rval)
{
    if (t == NULL || tidmap_size(&pct_state()->map) <= 1)
        return;

    pct_state()->counts.kmax++;

    if (!_pct_is_change_point(rval))
        return;

    /* in the paper: p[t] = d - i */
    t->priority = _pct_priority(rval);
    if (pct_state()->counts.chpts < pct_config()->chpts)
        pct_state()->counts.chpts++;
}

static int
_pct_cmp(const void *a, const void *b)
{
    const task_id *ta = (const task_id *)a;
    const task_id *tb = (const task_id *)b;

    const task_t *tta = (const task_t *)tidmap_find(&pct_state()->map, *ta);
    const task_t *ttb = (const task_t *)tidmap_find(&pct_state()->map, *tb);
    ASSERT(tta);
    ASSERT(ttb);

    return ttb->priority > tta->priority ? 1 :
           ttb->priority < tta->priority ? -1 :
           *ta > *tb                     ? 1 :
           *ta < *tb                     ? -1 :
                                           0;
}

static void
_pct_sort(tidset_t *tset)
{
    tidset_sort(tset, _pct_cmp);
}

/*******************************************************************************
 * handler
 ******************************************************************************/
STATIC void
_pct_handle(const capture_point *cp, event_t *e)
{
    once(pct_config()->enabled =
             (strcmp(sequencer_config()->strategy, "pct") == 0));
    ASSERT(e);
    ASSERT(cp);

    task_t *t = NULL;

    /* initialization */
    if (cp->type_id == EVENT_TASK_FINI) {
        tidmap_deregister(&pct_state()->map, cp->id);
        ASSERT(!tidset_has(&e->tset, cp->id));
    } else if (cp->type_id == EVENT_TASK_INIT) {
        t = (task_t *)tidmap_find(&pct_state()->map, cp->id);
        ASSERT(t == NULL);
        t = (task_t *)tidmap_register(&pct_state()->map, cp->id);
        ASSERT(t);
        t->priority = _pct_priority(prng_next());
    } else {
        t = (task_t *)tidmap_find(&pct_state()->map, cp->id);
        ASSERT(t);
    }

    if (!pct_config()->enabled || e->selector != SELECTOR_UNDEFINED ||
        e->readonly || tidset_size(&e->tset) <= 1 || !e->is_chpt || e->skip)
        return;

    if (t) {
        _pct_update(t, prng_next());
    }
    pct_state()->counts.kmax++;
    _pct_sort(&e->tset);
    e->selector = SELECTOR_FIRST;
    e->readonly = true;
    e->reason   = REASON_DETERMINISTIC;
    if (tidmap_size(&pct_state()->map) > pct_state()->counts.nmax)
        pct_state()->counts.nmax = tidmap_size(&pct_state()->map);
}
ON_SEQUENCER_CAPTURE(_pct_handle)
