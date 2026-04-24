
#include <lotto/base/tidset.h>
#include <lotto/engine/sequencer.h>
#include <lotto/engine/statemgr.h>
#include <lotto/modules/blocking/blocking.h>
#include <lotto/runtime/events.h>

static tidset_t _unblocked;

REGISTER_EPHEMERAL(_unblocked, { tidset_init(&_unblocked); })

STATIC void
_impasse_handle(const capture_point *cp, event_t *e)
{
    if (cp && cp->type_id == EVENT_TASK_CREATE) {
        return;
    }
    if (tidset_size(&e->tset) > 0 || any_blocked()) {
        tidset_copy(&_unblocked, &e->tset);
        return;
    }
    logger_errorf("Deadlock detected! (impasse)\n");
    e->reason = REASON_IMPASSE;
}
ON_SEQUENCER_CAPTURE(_impasse_handle);

const tidset_t *
available_or_soft_blocked_tasks()
{
    return &_unblocked;
}
