/*
 */
#define LOG_PREFIX LOG_CUR_FILE
#define LOG_BLOCK  LOG_CUR_BLOCK

#include <lotto/engine/dispatcher.h>
#include <lotto/engine/handlers/blocking.h>

STATIC void
_impasse_handle(const context_t *ctx, event_t *e)
{
    if (tidset_size(&e->tset) > 0 || any_blocked()) {
        return;
    }
    log_errorf("Deadlock detected! (impasse)\n");
    e->reason = REASON_IMPASSE;
}
REGISTER_HANDLER(SLOT_IMPASSE, _impasse_handle);
