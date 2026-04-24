#include "interceptors.h"
#include <lotto/modules/qemu/util.h>
#include <lotto/yield.h>

void
emit_wfi(unsigned int cpu_index, void *udata)
{
    (void)udata;
    if (!qemu_instrumentation_enabled(cpu_index)) {
        return;
    }
    lotto_yield(true);
}

/* Legacy compatibility while old symbol names are still referenced in-tree. */
void
vcpu_wfi_capture(unsigned int cpu_index, void *udata)
{
    emit_wfi(cpu_index, udata);
}
