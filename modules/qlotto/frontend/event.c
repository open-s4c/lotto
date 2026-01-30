/*
 */

#include <stdint.h>
#include <string.h>

#include <lotto/base/context.h>
#include <lotto/brokers/statemgr.h>
#include <lotto/qlotto/frontend/event.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/memory.h>

/*******************************************************************************
 * Events
 *******************************************************************************/

void
qlotto_add_event(map_t *emap, uint64_t e_pc, category_t cat, char *func_name)
{
    eventi_t *event = (eventi_t *)map_find(emap, e_pc);
    if (NULL != event)
        return;

    event            = (eventi_t *)map_register(emap, e_pc);
    event->cat       = cat;
    event->func_name = func_name;
    // logger_infof( "Event added 0x%lx\n", b_pc);
}

void
qlotto_del_event(map_t *emap, uint64_t e_pc)
{
    map_deregister(emap, e_pc);
}

eventi_t *
qlotto_get_event(map_t *emap, uint64_t e_pc)
{
    eventi_t *event = (eventi_t *)map_find(emap, e_pc);
    return event;
}
