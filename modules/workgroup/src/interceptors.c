#include "state.h"
#include <dice/chains/intercept.h>
#include <dice/module.h>
#include <dice/pubsub.h>
#include <lotto/modules/workgroup/events.h>
#include <lotto/runtime/capture_point.h>
#include <lotto/runtime/ingress.h>
#include <lotto/workgroup.h>

FORWARD_CAPTURE_TO_INGRESS(EVENT, EVENT_WORKGROUP_JOIN)

void
_lotto_workgroup_join(void)
{
    capture_point cp = {
        .chain_id = INTERCEPT_EVENT,
        .type_id  = EVENT_WORKGROUP_JOIN,
        .func     = "lotto_workgroup_join",
    };
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_WORKGROUP_JOIN, &cp, 0);
    workgroup_clear_current(NULL);
}
