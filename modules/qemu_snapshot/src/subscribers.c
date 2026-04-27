#include <dice/pubsub.h>
#include <lotto/engine/pubsub.h>
#include <lotto/modules/qemu/events.h>
#include <lotto/modules/qemu_snapshot/events.h>
#include <lotto/qemu/trap.h>

void qemu_snapshot_trap(const struct lotto_trap_event *ev);
void qemu_snapshot_plugin_init(const qemu_control_event_t *ev);
void qemu_snapshot_plugin_fini(const qemu_control_event_t *ev);

PS_SUBSCRIBE(CHAIN_QEMU_CONTROL, EVENT_QEMU_INIT, {
    qemu_snapshot_plugin_init((const qemu_control_event_t *)event);
})

PS_SUBSCRIBE(CHAIN_QEMU_CONTROL, EVENT_QEMU_FINI, {
    qemu_snapshot_plugin_fini((const qemu_control_event_t *)event);
})

PS_SUBSCRIBE(CHAIN_LOTTO_TRAP, EVENT_QEMU_SNAPSHOT, {
    qemu_snapshot_trap((const struct lotto_trap_event *)event);
    return PS_OK;
})
