#define LOGGER_PREFIX LOGGER_CUR_FILE
#define LOGGER_BLOCK  LOGGER_CUR_BLOCK
#include <dice/module.h>
#include <lotto/brokers/statemgr.h>
#include <lotto/modules/blocking/state.h>
#include <lotto/sys/logger_block.h>


/*******************************************************************************
 * persistent state
 ******************************************************************************/

static tidset_t _returned;

STATIC void _returned_print(const marshable_t *m);

REGISTER_PERSISTENT(_returned, {
    tidset_init(&_returned);
    _returned.m.print = _returned_print;
})

/*******************************************************************************
 * marshaling implementation
 ******************************************************************************/

STATIC void
_returned_print(const marshable_t *m)
{
    logger_infof("returned tasks: ");
    tidset_print(m);
}

/*******************************************************************************
 * getter
 ******************************************************************************/
tidset_t *
returned_tasks()
{
    return &_returned;
}
