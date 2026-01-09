/*
 */
#define LOG_PREFIX LOG_CUR_FILE
#define LOG_BLOCK  LOG_CUR_BLOCK
#include <lotto/brokers/statemgr.h>
#include <lotto/states/handlers/blocking.h>
#include <lotto/sys/logger_block.h>
#include <lotto/util/macros.h>

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
