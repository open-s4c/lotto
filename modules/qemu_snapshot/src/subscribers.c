#include <dice/pubsub.h>
#include <lotto/engine/pubsub.h>
#include <lotto/modules/qemu/events.h>

void qemu_snapshot_tb_translate(const qemu_translate_event_t *ev);
void qemu_snapshot_plugin_init(const qemu_control_event_t *ev);
void qemu_snapshot_plugin_fini(const qemu_control_event_t *ev);

PS_SUBSCRIBE(CHAIN_QEMU_CONTROL, EVENT_QEMU_INIT, {
    qemu_snapshot_plugin_init((const qemu_control_event_t *)event);
})

PS_SUBSCRIBE(CHAIN_QEMU_CONTROL, EVENT_QEMU_FINI, {
    qemu_snapshot_plugin_fini((const qemu_control_event_t *)event);
})

PS_SUBSCRIBE(CHAIN_QEMU_CONTROL, EVENT_QEMU_TRANSLATE, {
    qemu_snapshot_tb_translate((const qemu_translate_event_t *)event);
})
