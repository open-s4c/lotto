#include <dice/pubsub.h>
#include <lotto/engine/pubsub.h>
#include <lotto/modules/qemu/events.h>

void qemu_gdb_translate(const qemu_translate_event_t *ev);
void qemu_gdb_plugin_init(const qemu_control_event_t *ev);
void qemu_gdb_plugin_fini(const qemu_control_event_t *ev);

PS_SUBSCRIBE(CHAIN_QEMU_CONTROL, EVENT_QEMU_INIT,
             { qemu_gdb_plugin_init((const qemu_control_event_t *)event); })

PS_SUBSCRIBE(CHAIN_QEMU_CONTROL, EVENT_QEMU_FINI,
             { qemu_gdb_plugin_fini((const qemu_control_event_t *)event); })

PS_SUBSCRIBE(CHAIN_QEMU_CONTROL, EVENT_QEMU_TRANSLATE,
             { qemu_gdb_translate((const qemu_translate_event_t *)event); })
