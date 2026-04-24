#include "interceptors.h"
#include "qemu_event.h"
#include <lotto/base/reason.h>
#include <lotto/modules/qemu/events.h>
#include <lotto/modules/qemu/util.h>
#include <lotto/runtime/capture_point.h>
#include <lotto/runtime/runtime.h>

void
emit_udf_exit(unsigned int cpu_index, void *udata)
{
    (void)cpu_index;
    capture_point *cp = (capture_point *)udata;
    if (cp == NULL || cp->type_id != EVENT_QLOTTO_EXIT || cp->payload == NULL) {
        return;
    }

    qlotto_exit_event_t *ev = (qlotto_exit_event_t *)cp->payload;
    if (ev->reason == REASON_SUCCESS) {
        lotto_exit(cp, REASON_SUCCESS);
        return;
    }
    if (ev->reason == REASON_ASSERT_FAIL) {
        lotto_exit(cp, REASON_SIGABRT);
        return;
    }
    lotto_exit(cp, ev->reason);
}

/* Legacy compatibility while old symbol names are still referenced in-tree. */
void
vcpu_exit_capture(unsigned int cpu_index, void *udata)
{
    emit_udf_exit(cpu_index, udata);
}
