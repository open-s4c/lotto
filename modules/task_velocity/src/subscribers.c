#include <dice/chains/capture.h>
#include <dice/module.h>
#include <dice/pubsub.h>
#include <lotto/engine/pubsub.h>
#include <lotto/modules/task_velocity/events.h>
#include <lotto/runtime/capture_point.h>

PS_SUBSCRIBE(CAPTURE_EVENT, EVENT_TASK_VELOCITY, {
    task_velocity_event_t *ev = EVENT_PAYLOAD(event);
    capture_point cp          = {
                 .type_id = EVENT_TASK_VELOCITY,
                 .payload = ev,
                 .func    = __FUNCTION__,
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_TASK_VELOCITY, &cp, md);
    return PS_OK;
})
