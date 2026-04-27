#include "state.h"

void
_workgroup_handle(const capture_point *cp, event_t *e)
{
    workgroup_handle_capture(cp, e);
}

ON_SEQUENCER_CAPTURE(_workgroup_handle)

LOTTO_SUBSCRIBE_SEQUENCER_RESUME(ANY_EVENT, {
    const capture_point *cp = (const capture_point *)md;
    if (cp == NULL) {
        return PS_OK;
    }

    workgroup_handle_resume(cp);
})
