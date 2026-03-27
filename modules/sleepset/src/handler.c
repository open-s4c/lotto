#include <stdint.h>

#include "state.h"
#include <dice/events/memaccess.h>
#include <lotto/engine/sequencer.h>
#include <lotto/runtime/events.h>
#include <lotto/runtime/ingress_events.h>
#include <lotto/sys/assert.h>

static bool _extract_access(const capture_point *cp, sleepset_access_t *access);
static void _record_task_access(task_id id, const sleepset_access_t *access);
static void _drop_task_state(task_id id);
static bool _accesses_commute(const sleepset_access_t *lhs,
                              const sleepset_access_t *rhs);

static inline bool
_is_read_access(type_id type)
{
    switch (type) {
        case EVENT_MA_READ:
        case EVENT_MA_AREAD:
        case EVENT_MA_READ_RANGE:
            return true;
        default:
            return false;
    }
}

static inline bool
_is_write_access(type_id type)
{
    switch (type) {
        case EVENT_MA_WRITE:
        case EVENT_MA_AWRITE:
        case EVENT_MA_RMW:
        case EVENT_MA_XCHG:
        case EVENT_MA_CMPXCHG:
        case EVENT_MA_CMPXCHG_WEAK:
        case EVENT_MA_WRITE_RANGE:
            return true;
        default:
            return false;
    }
}

static uintptr_t
_range_end(uintptr_t addr, size_t size)
{
    if (size > UINTPTR_MAX - addr)
        return UINTPTR_MAX;
    return addr + size;
}

static bool
_ranges_overlap(const sleepset_access_t *lhs, const sleepset_access_t *rhs)
{
    uintptr_t lend = _range_end(lhs->addr, lhs->size);
    uintptr_t rend = _range_end(rhs->addr, rhs->size);
    return lhs->addr < rend && rhs->addr < lend;
}

static bool
_extract_access(const capture_point *cp, sleepset_access_t *access)
{
    ASSERT(access);
    *access = (sleepset_access_t){0};

    if (_is_read_access(cp->src_type)) {
        access->kind = SLEEPSET_ACCESS_READ;
    } else if (_is_write_access(cp->src_type)) {
        access->kind = SLEEPSET_ACCESS_WRITE;
    } else {
        return false;
    }

    if (!has_memaccess_addr(cp))
        return false;

    access->addr  = memaccess_addr(cp);
    access->size  = memaccess_size(cp);
    access->valid = true;
    return true;
}

static sleepset_task_access_t *
_find_access(task_id id)
{
    return (sleepset_task_access_t *)tidmap_find(&sleepset_state()->accesses,
                                                 id);
}

static void
_record_task_access(task_id id, const sleepset_access_t *access)
{
    ASSERT(id != NO_TASK);
    if (access == NULL || !access->valid) {
        tiditem_t *it = tidmap_find(&sleepset_state()->accesses, id);
        if (it) {
            tidmap_deregister(&sleepset_state()->accesses, id);
        }
        return;
    }

    sleepset_task_access_t *entry = _find_access(id);
    if (entry == NULL) {
        entry = (sleepset_task_access_t *)tidmap_register(
            &sleepset_state()->accesses, id);
    }
    ASSERT(entry);
    entry->access = *access;
}

static void
_drop_task_state(task_id id)
{
    tidset_remove(&sleepset_state()->sleeping, id);
    tidset_remove(&sleepset_state()->prev_enabled, id);
    tidset_remove(&sleepset_state()->cur_enabled, id);
    tidset_remove(&sleepset_state()->next_sleeping, id);
    _record_task_access(id, NULL);
}

static bool
_accesses_commute(const sleepset_access_t *lhs, const sleepset_access_t *rhs)
{
    ASSERT(lhs);
    ASSERT(rhs);
    if (!lhs->valid || !rhs->valid)
        return false;

    if (!_ranges_overlap(lhs, rhs))
        return true;

    return lhs->kind == SLEEPSET_ACCESS_READ &&
           rhs->kind == SLEEPSET_ACCESS_READ;
}

static void
_update_sleepset(const capture_point *cp)
{
    sleepset_state_t *state = sleepset_state();
    sleepset_access_t current;
    tidset_clear(&state->next_sleeping);

    /*
     * Lotto does not expose the next semantic action for every enabled task at
     * a capture. We therefore approximate TMC's task metadata with the last
     * memaccess captured for each task.
     */
    if (!_extract_access(cp, &current)) {
        tidset_clear(&state->sleeping);
        return;
    }

    /*
     * Read-driven sleeping is too aggressive for this last-access
     * approximation: spin loops can repeatedly read and incorrectly sleep the
     * writer/unlocker that is needed for progress.
     */
    if (current.kind == SLEEPSET_ACCESS_READ) {
        tidset_clear(&state->sleeping);
        return;
    }

    size_t chosen_idx = tidset_size(&state->prev_enabled);
    bool found_self   = false;
    for (size_t i = 0; i < tidset_size(&state->prev_enabled); i++) {
        if (tidset_get(&state->prev_enabled, i) == cp->id) {
            chosen_idx = i;
            found_self = true;
            break;
        }
    }

    for (size_t i = 0; i < tidset_size(&state->prev_enabled); i++) {
        task_id tid                   = tidset_get(&state->prev_enabled, i);
        sleepset_task_access_t *entry = _find_access(tid);
        if (entry == NULL || !_accesses_commute(&current, &entry->access)) {
            continue;
        }

        if (tidset_has(&state->sleeping, tid) ||
            (found_self && i < chosen_idx)) {
            tidset_insert(&state->next_sleeping, tid);
        }
    }

    /*
     * The currently executing task must never remain asleep for the current
     * pruning step, even if the carried approximation still contains it.
     */
    tidset_remove(&state->next_sleeping, cp->id);
    tidset_copy(&state->sleeping, &state->next_sleeping);
}

static void
_filter_sleeping_set(const capture_point *cp, event_t *e)
{
    const tidset_t *sleeping = &sleepset_state()->sleeping;
    for (size_t i = 0; i < tidset_size(sleeping); i++) {
        task_id id = tidset_get(sleeping, i);
        if (id == cp->id) {
            continue;
        }
        tidset_remove(&e->tset, id);
    }
}

void
sleepset_handle(const capture_point *cp, event_t *e)
{
    ASSERT(cp);
    ASSERT(e);

    if (!sleepset_config()->enabled || e->skip)
        return;

    tidset_copy(&sleepset_state()->cur_enabled, &e->tset);
    _update_sleepset(cp);
    _filter_sleeping_set(cp, e);

    if (cp->src_type == EVENT_TASK_FINI) {
        _drop_task_state(cp->id);
    } else {
        sleepset_access_t access;
        bool has_access = _extract_access(cp, &access);
        _record_task_access(cp->id, has_access ? &access : NULL);
    }

    tidset_copy(&sleepset_state()->prev_enabled,
                &sleepset_state()->cur_enabled);
}
ON_SEQUENCER_CAPTURE(sleepset_handle)
