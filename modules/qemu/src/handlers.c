#include <lotto/engine/dispatcher.h>
#include <lotto/engine/pubsub.h>
#include <lotto/modules/deadlock/events.h>
#include <lotto/qlotto/frontend/interceptor.h>
#include <lotto/qlotto/gdb/halter.h>
#include <lotto/sys/stdio.h>

static bool warned_capture_not_ready_;
static bool warned_gdb_not_ready_;

static void
_qemu_capture_handle(const context_t *ctx, event_t *e)
{
    if (!qlotto_frontend_ready()) {
        if (!warned_capture_not_ready_) {
            warned_capture_not_ready_ = true;
            sys_fprintf(stderr,
                        "warning: qlotto frontend not ready; ignoring early capture\n");
        }
        return;
    }

    qlotto_capture_handle(ctx, e);
}
REGISTER_HANDLER_EXTERNAL(_qemu_capture_handle)

LOTTO_SUBSCRIBE(EVENT_DEADLOCK__DETECTED, {
    if (!qlotto_gdb_ready()) {
        if (!warned_gdb_not_ready_) {
            warned_gdb_not_ready_ = true;
            sys_fprintf(stderr,
                        "warning: qlotto gdb plugin not ready; ignoring early deadlock event\n");
        }
    } else {
        qlotto_deadlock_detected();
    }
})
