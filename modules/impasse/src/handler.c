
#include <lotto/engine/sequencer.h>
#include <lotto/modules/blocking/blocking.h>

STATIC void
_impasse_handle(const capture_point *cp, event_t *e)
{
    (void)cp;
    if (tidset_size(&e->tset) > 0 || any_blocked()) {
        return;
    }
    logger_errorf("Deadlock detected! (impasse)\n");
    e->reason = REASON_IMPASSE;
}
ON_SEQUENCER_CAPTURE(_impasse_handle);
