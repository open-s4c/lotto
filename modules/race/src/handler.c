#include <stddef.h>
#include <string.h>

#include "state.h"
#include <lotto/base/tidmap.h>
#include <lotto/engine/sequencer.h>
#include <lotto/engine/statemgr.h>
#include <lotto/modules/ichpt/ichpt.h>
#include <lotto/modules/mutex/events.h>
#include <lotto/modules/race/race_result.h>
#include <lotto/runtime/events.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger.h>
#include <lotto/util/macros.h>

/* deregister item in tidmap after 100 clk ticks */
#ifdef RACE_DEFAULT
    #define LAZY_DEREG_DELAY 100
    #define MAX_CAPACITY     100
#else
    #define LAZY_DEREG_DELAY 10
    #define MAX_CAPACITY     4
    // #define MASK             0xFFFF000000000000
    #define MASK             0xFFFFFFFF00000000
#endif

typedef struct {
    task_id id;
    bool virtual; ///< whether ID is a vid

    uintptr_t addr;
    uintptr_t pc;
    bool readonly;
    bool atomic;
} ot_entry;

typedef struct {
    tiditem_t t;
    clk_t dereg_at;
    size_t capacity;
    size_t size;
    ot_entry entries[MAX_CAPACITY];
} ot_set;


static tidmap_t _state;
static void
race_reset()
{
    const tiditem_t *i = NULL;
    while ((i = tidmap_iterate(&_state)) != NULL)
        tidmap_deregister(&_state, i->key);

    tidmap_init(&_state, MARSHABLE_STATIC(sizeof(ot_set)));
}
REGISTER_EPHEMERAL(_state, { race_reset(); })

/*******************************************************************************
 * OT functions
 ******************************************************************************/
static ot_set *
ot_get_or_reg(task_id id)
{
    tiditem_t *item = tidmap_find(&_state, id);
    if (item == NULL)
        item = tidmap_register(&_state, id);
    return (ot_set *)item;
}

static void
ot_lazy_dereg(task_id id, clk_t clk)
{
    tiditem_t *item = tidmap_find(&_state, id);
    if (item == NULL)
        return;
    ot_set *oset = (ot_set *)item;

    /* if already marked for deletion, return */
    if (oset->dereg_at != 0)
        return;

    oset->dereg_at = clk + LAZY_DEREG_DELAY;
}

static void
ot_add(ot_set *oset, ot_entry *e)
{
    if (oset->size >= MAX_CAPACITY)
        return;
    ASSERT(e->addr != 0);
    ot_entry *ee = &oset->entries[oset->size++];
    *ee          = *e;
}

static void
ot_cleanup(clk_t clk)
{
    const tiditem_t *cur = tidmap_iterate(&_state);
    for (; cur; cur = tidmap_next(cur)) {
        ot_set *oset = (ot_set *)cur;
        ASSERT(oset->t.key != NO_TASK);
        if (oset->dereg_at != 0 && oset->dereg_at <= clk)
            tidmap_deregister(&_state, cur->key);
    }
}

static race_t
ot_conflict(ot_set *this, ot_entry *e, clk_t clk)
{
    ASSERT(this->t.key != NO_TASK);
    for (const tiditem_t *cur = tidmap_iterate(&_state); cur;) {
        ot_set *oset = (ot_set *)cur;
        ASSERT(oset->t.key != NO_TASK);
        if (this->t.key == oset->t.key) {
            cur = tidmap_next(cur);
            continue;
        }
        if (oset->dereg_at != 0 && oset->dereg_at <= clk) {
            task_id id = cur->key;
            cur        = tidmap_next(cur);
            tidmap_deregister(&_state, id);
            continue;
        }
        for (size_t j = 0; j < oset->size; j++) {
            ot_entry *ee = &oset->entries[j];

            if (ee->addr != e->addr) {
                continue;
            }

            if (ee->virtual != e->virtual) {
                /* only compare entries if they are both in userspace (with vid)
                 * or both in kernel space (both without vid) */
                continue;
            }

            if (ee->atomic && e->atomic) {
                /* if both entries are atomic, then this is not a data race, but
                 * an intended race */
                continue;
            }

            if (race_config()->ignore_write_write && !ee->readonly &&
                !e->readonly) {
                /* if both entries are writing, ignore such race */
                continue;
            }

            if (ee->readonly && e->readonly) {
                continue;
            }

            return (race_t){
                .addr = e->addr,
                .loc1 =
                    (struct race_loc){
                        .id       = e->id,
                        .pc       = e->pc,
                        .readonly = e->readonly,
                    },
                .loc2 =
                    (struct race_loc){
                        .id       = ee->id,
                        .pc       = ee->pc,
                        .readonly = ee->readonly,
                    },
            };
        }
        cur = tidmap_next(cur);
    }
    return (race_t){0};
}

static void
ot_clear(ot_set *oset)
{
    oset->size = 0;
}

/*******************************************************************************
 * public interface
 ******************************************************************************/
void
race_print(race_t race)
{
    logger_infof(
        "RACE: T%lx and T%lx at addr=0x%lx between (instr = {0x%lx, "
        "0x%lx})\n",
        race.loc1.id, race.loc2.id, race.addr, race.loc1.pc, race.loc2.pc);
}

race_t
race_check(const capture_point *cp, clk_t clk)
{
    race_t race = {0};
    ot_entry e  = {
         .addr    = 0,
         .id      = cp->vid != NO_TASK ? cp->vid : cp->id,
         .virtual = cp->vid != NO_TASK,
         .pc      = cp->pc,
    };

    ot_set *oset = ot_get_or_reg(e.id);
    switch (cp->src_chain) {
        default: // BEFORE or EVENT
            switch (cp->src_type) {
                case EVENT_MA_AREAD:
                    e.atomic = true;
                    // fallthru
                case EVENT_MA_READ:
                    e.readonly = true;
                    // fallthru
                case EVENT_MA_WRITE:
                    e.addr = memaccess_addr(cp);
                    if (e.addr == 0)
                        return race;
#ifndef RACE_DEFAULT
                    if (e.addr & MASK)
                        return race;
#endif

                    ot_add(oset, &e);
                    race = ot_conflict(oset, &e, clk);
                    if (race.addr)
                        return race;
                    break;

                case EVENT_MA_AWRITE:
                case EVENT_MA_CMPXCHG:
                case EVENT_MA_XCHG:
                case EVENT_MA_RMW:
                    e.addr = memaccess_addr(cp);
                    if (e.addr == 0)
                        return race;
#ifndef RACE_DEFAULT
                    if (e.addr & MASK)
                        return race;
#endif

                    e.atomic   = true;
                    e.readonly = false;
                    race       = ot_conflict(oset, &e, clk);
                    if (race.addr)
                        return race;
                    break;
                default:
                    break;
            }
            break;

        case CHAIN_INGRESS_AFTER:
            switch (cp->src_type) {
                case EVENT_MA_CMPXCHG:
                case EVENT_MA_AWRITE:
                case EVENT_MA_XCHG:
                case EVENT_MA_RMW:
                case EVENT_MA_FENCE:
                    ot_clear(oset);
                    break;
                default:
                    break;
            }
            break;
    }
    return race;
}

/*******************************************************************************
 * handler
 ******************************************************************************/
STATIC void
_race_handle(const capture_point *cp, event_t *e)
{
    ASSERT(cp && cp->id != NO_TASK);
    ASSERT(e);

    if (!race_config()->enabled)
        return;

#if defined(QLOTTO_ENABLED)
    if (((cp->pstate >> 2) & 0x3) != 0)
        return;
#endif

    if (cp->src_type == EVENT_TASK_FINI) {
        ot_lazy_dereg(cp->vid != NO_TASK ? cp->vid : cp->id, e->clk);
        return;
    }
    if (cp->src_type == EVENT_TASK_INIT) {
        return;
    }

    race_t race = race_check(cp, e->clk);
    if (race.addr == 0) {
        /* do some cleanup every couple of clks */
        if ((e->clk % LAZY_DEREG_DELAY) == 0)
            ot_cleanup(e->clk);
        return;
    }

    if (race_config()->abort_on_race) {
        logger_errorf("Data race detected at addr: %p (strict mode)\n",
                      (void *)race.addr);
        logger_fatalf("(tid: %lx instr: %p) and (tid: %lx instr: %p)\n",
                      race.loc1.id, (void *)race.loc1.pc, race.loc2.id,
                      (void *)race.loc2.pc);
    } else
        race_print(race);
    if (race_config()->only_write_ichpt) {
        if (!race.loc1.readonly)
            add_ichpt(race.loc1.pc);
        if (!race.loc2.readonly)
            add_ichpt(race.loc2.pc);
    } else {
        add_ichpt(race.loc1.pc);
        if (race.loc1.pc != race.loc2.pc)
            add_ichpt(race.loc2.pc);
    }
}
ON_SEQUENCER_CAPTURE(_race_handle)

STATIC void
_race_resume_handle(const capture_point *cp, event_t *e)
{
    (void)e;
    ASSERT(cp && cp->id != NO_TASK);

    if (!race_config()->enabled)
        return;
    switch (cp->src_type) {
        default:
            return;
        case EVENT_TASK_CREATE:
        case EVENT_MUTEX_RELEASE:
            break;
    }

    task_id id = cp->vid != NO_TASK ? cp->vid : cp->id;

    tiditem_t *item = tidmap_find(&_state, id);
    if (item == NULL)
        return;

    ot_clear((ot_set *)item);
}
ON_SEQUENCER_RESUME(_race_resume_handle)
