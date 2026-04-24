#include <dice/chains/capture.h>
#include <dice/pubsub.h>
#include <lotto/engine/pubsub.h>
#include <lotto/modules/qemu/events.h>
#include <lotto/modules/qemu_snapshot/events.h>
#include <lotto/qemu/trap.h>
#include <lotto/runtime/capture_point.h>

void qemu_snapshot_trap(const capture_point *trap_cp, metadata_t *md);
void qemu_snapshot_plugin_init(const qemu_control_event_t *ev);
void qemu_snapshot_plugin_fini(const qemu_control_event_t *ev);

PS_SUBSCRIBE(CHAIN_QEMU_CONTROL, EVENT_QEMU_INIT, {
    qemu_snapshot_plugin_init((const qemu_control_event_t *)event);
})

PS_SUBSCRIBE(CHAIN_QEMU_CONTROL, EVENT_QEMU_FINI, {
    qemu_snapshot_plugin_fini((const qemu_control_event_t *)event);
})

PS_SUBSCRIBE(CAPTURE_EVENT, EVENT_QEMU_SNAPSHOT_DONE, {
    struct qemu_snapshot_done_event *ev = EVENT_PAYLOAD(event);
    capture_point cp                    = {
                           .type_id = EVENT_QEMU_SNAPSHOT_DONE,
                           .payload = ev,
                           .func    = __FUNCTION__,
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_QEMU_SNAPSHOT_DONE, &cp, md);
    return PS_OK;
})

PS_SUBSCRIBE(CHAIN_LOTTO_TRAP, EVENT_QEMU_SNAPSHOT_REQUEST, {
    qemu_snapshot_trap((const capture_point *)event, md);
    return PS_OK;
})
