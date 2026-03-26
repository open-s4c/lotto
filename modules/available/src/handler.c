#include <lotto/engine/sequencer.h>
#include <lotto/modules/available/state.h>
#include <lotto/sys/logger.h>
#include <lotto/util/macros.h>

STATIC void
_available_handle(const capture_point *cp, event_t *e)
{
    (void)cp;
    if (!available_config()->enabled) {
        return;
    }
    ASSERT(e);
    tidset_copy(get_available_tasks(), &e->tset);
}
ON_SEQUENCER_CAPTURE(_available_handle);
