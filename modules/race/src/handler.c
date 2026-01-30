/*
 */
#include <stddef.h>
#include <string.h>

#define LOGGER_PREFIX LOGGER_CUR_FILE
#define LOGGER_BLOCK  LOGGER_CUR_BLOCK
#include <lotto/base/tidmap.h>
#include <lotto/brokers/statemgr.h>
#include <lotto/engine/dispatcher.h>
#include <lotto/engine/handlers/ichpt.h>
#include <lotto/engine/handlers/race.h>
#include <lotto/states/handlers/race.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger_block.h>
#include <lotto/util/macros.h>

/* deregister item in tidmap after 100 clk ticks */
#ifdef RACE_DEFAULT
    #define LAZY_DEREG_DELAY 100
    #define MAX_CAPACITY     100
#else
    #define LAZY_DEREG_DELAY 10
    #define MAX_CAPACITY     4
    //#define MASK             0xFFFF000000000000
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
race_check(const context_t *ctx, clk_t clk)
{
    const category_t cat = ctx->cat;
    race_t race          = {0};
    ot_entry e           = {
                  .addr    = ctx->args[0].value.ptr,
                  .id      = ctx->vid != NO_TASK ? ctx->vid : ctx->id,
                  .virtual = ctx->vid != NO_TASK,
                  .pc      = ctx->pc,
    };

#ifndef RACE_DEFAULT
    if (e.addr & MASK)
        return race;
#endif

    ot_set *oset = ot_get_or_reg(e.id);
    switch (cat) {
        case CAT_BEFORE_AREAD:
            e.atomic = true;
            // fallthru
        case CAT_BEFORE_READ:
            e.readonly = true;
            // fallthru
        case CAT_BEFORE_WRITE:
            if (e.addr == 0)
                return race;

            ot_add(oset, &e);
            race = ot_conflict(oset, &e, clk);
            if (race.addr)
                return race;
            break;

        case CAT_BEFORE_AWRITE:
        case CAT_BEFORE_CMPXCHG:
        case CAT_BEFORE_XCHG:
        case CAT_BEFORE_RMW:
            if (e.addr == 0)
                return race;

            e.atomic   = true;
            e.readonly = false;
            race       = ot_conflict(oset, &e, clk);
            if (race.addr)
                return race;
            break;

        case CAT_AFTER_CMPXCHG_F:
        case CAT_AFTER_CMPXCHG_S:
        case CAT_AFTER_AWRITE:
        case CAT_AFTER_XCHG:
        case CAT_AFTER_RMW:
        case CAT_AFTER_FENCE:
            ot_clear(oset);
            break;
        default:
            break;
    }
    return race;
}

/*******************************************************************************
 * handler
 ******************************************************************************/
STATIC void
_race_handle(const context_t *ctx, event_t *e)
{
    ASSERT(ctx && ctx->id != NO_TASK);
    ASSERT(e);

    if (!race_config()->enabled)
        return;

#if defined(QLOTTO_ENABLED)
    if (((ctx->pstate >> 2) & 0x3) != 0)
        return;
#endif

    switch (ctx->cat) {
        case CAT_TASK_FINI:
            ot_lazy_dereg(ctx->vid != NO_TASK ? ctx->vid : ctx->id, e->clk);
            // fallthru
        case CAT_TASK_INIT:
            return;
        default:
            break;
    }

    race_t race = race_check(ctx, e->clk);
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
REGISTER_HANDLER(SLOT_RACE, _race_handle)
