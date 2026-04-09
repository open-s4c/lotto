#include <string.h>

#include "state.h"
#include <dice/events/memaccess.h>
#include <lotto/engine/pubsub.h>
#include <lotto/engine/sequencer.h>
#include <lotto/runtime/capture_point.h>
#include <lotto/runtime/ingress_events.h>
#include <lotto/sys/ensure.h>

void sleepset_handle(const capture_point *cp, event_t *e);

#define ADDR(V) ((uintptr_t)(V))

#define cp_read(ID, ADDR_)                                                     \
    (capture_point)                                                            \
    {                                                                          \
        .id = (ID), .vid = NO_TASK, .src_chain = CHAIN_INGRESS_EVENT,          \
        .src_type = EVENT_MA_READ,                                             \
        .payload  = &(struct ma_read_event){.addr = (void *)(ADDR_),           \
                                            .size = sizeof(uintptr_t)},        \
    }

#define cp_write(ID, ADDR_)                                                    \
    (capture_point)                                                            \
    {                                                                          \
        .id = (ID), .vid = NO_TASK, .src_chain = CHAIN_INGRESS_EVENT,          \
        .src_type = EVENT_MA_WRITE,                                            \
        .payload  = &(struct ma_write_event){.addr = (void *)(ADDR_),          \
                                             .size = sizeof(uintptr_t)},       \
    }

static void
event_init(event_t *e, task_id first, task_id second)
{
    memset(e, 0, sizeof(*e));
    tidset_init(&e->tset);
    ENSURE(tidset_insert(&e->tset, first));
    ENSURE(tidset_insert(&e->tset, second));
}

static void
event_fini(event_t *e)
{
    tidset_fini(&e->tset);
}

static void
test_commuting_write_adds_sleeping_task()
{
    sleepset_reset();
    sleepset_config()->enabled = true;

    event_t e1;
    event_init(&e1, 1, 2);
    capture_point cp1 = cp_read(1, ADDR(0x1000));
    sleepset_handle(&cp1, &e1);
    ENSURE(tidset_size(&e1.tset) == 2);
    event_fini(&e1);

    event_t e2;
    event_init(&e2, 1, 2);
    capture_point cp2 = cp_write(2, ADDR(0x2000));
    sleepset_handle(&cp2, &e2);
    ENSURE(tidset_size(&e2.tset) == 1);
    ENSURE(tidset_has(&e2.tset, 2));
    ENSURE(tidset_has(&sleepset_state()->sleeping, 1));
    event_fini(&e2);
}

static void
test_read_does_not_carry_sleeping_tasks()
{
    sleepset_reset();
    sleepset_config()->enabled = true;

    event_t e1;
    event_init(&e1, 1, 2);
    capture_point cp1 = cp_read(1, ADDR(0x1000));
    sleepset_handle(&cp1, &e1);
    event_fini(&e1);

    event_t e2;
    event_init(&e2, 1, 2);
    capture_point cp2 = cp_read(2, ADDR(0x2000));
    sleepset_handle(&cp2, &e2);
    ENSURE(tidset_size(&e2.tset) == 2);
    ENSURE(!tidset_has(&sleepset_state()->sleeping, 1));
    event_fini(&e2);
}

static void
test_conflicting_access_keeps_candidate_enabled()
{
    sleepset_reset();
    sleepset_config()->enabled = true;

    event_t e1;
    event_init(&e1, 1, 2);
    capture_point cp1 = cp_read(1, ADDR(0x3000));
    sleepset_handle(&cp1, &e1);
    event_fini(&e1);

    event_t e2;
    event_init(&e2, 1, 2);
    capture_point cp2 = cp_write(2, ADDR(0x3000));
    sleepset_handle(&cp2, &e2);
    ENSURE(tidset_size(&e2.tset) == 2);
    ENSURE(tidset_has(&e2.tset, 1));
    ENSURE(tidset_has(&e2.tset, 2));
    ENSURE(!tidset_has(&sleepset_state()->sleeping, 1));
    event_fini(&e2);
}

static void
test_current_task_is_never_pruned_by_carried_sleeping_state()
{
    sleepset_reset();
    sleepset_config()->enabled = true;

    ENSURE(tidset_insert(&sleepset_state()->sleeping, 2));
    ENSURE(tidset_insert(&sleepset_state()->prev_enabled, 1));
    ENSURE(tidset_insert(&sleepset_state()->prev_enabled, 2));

    event_t e;
    event_init(&e, 1, 2);
    capture_point cp = cp_read(2, ADDR(0x4000));
    sleepset_handle(&cp, &e);
    ENSURE(tidset_has(&e.tset, 2));
    ENSURE(!tidset_has(&sleepset_state()->sleeping, 2));
    event_fini(&e);
}

int
main()
{
    START_REGISTRATION_PHASE();
    test_commuting_write_adds_sleeping_task();
    test_read_does_not_carry_sleeping_tasks();
    test_conflicting_access_keeps_candidate_enabled();
    test_current_task_is_never_pruned_by_carried_sleeping_state();
    return 0;
}
