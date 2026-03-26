#include "state.h"
#include <dice/module.h>
#include <lotto/engine/statemgr.h>
#include <lotto/sys/logger.h>

static tidset_t _returned;

STATIC void _returned_print(const marshable_t *m);

REGISTER_PERSISTENT(_returned, {
    tidset_init(&_returned);
    _returned.m.print = _returned_print;
})

STATIC void
_returned_print(const marshable_t *m)
{
    logger_infof("returned tasks: ");
    tidset_print(m);
}

tidset_t *
returned_tasks()
{
    return &_returned;
}
