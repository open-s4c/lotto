#define LOGGER_BLOCK LOGGER_CUR_BLOCK

#include <lotto/engine/dispatcher.h>
#include <lotto/modules/blocking/blocking.h>

STATIC void
_impasse_handle(const context_t *ctx, event_t *e)
{
    if (tidset_size(&e->tset) > 0 || any_blocked()) {
        return;
    }
    logger_errorf("Deadlock detected! (impasse)\n");
    e->reason = REASON_IMPASSE;
}
REGISTER_HANDLER(_impasse_handle);
