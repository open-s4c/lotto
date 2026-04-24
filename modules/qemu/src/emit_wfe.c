#include "interceptors.h"
#include <lotto/modules/qemu/util.h>
#include <lotto/yield.h>

void
emit_wfe(unsigned int cpu_index, void *udata)
{
    (void)udata;
    if (!qemu_instrumentation_enabled(cpu_index)) {
        return;
    }
    lotto_yield(true);
}

/* Legacy compatibility while old symbol names are still referenced in-tree. */
void
vcpu_wfe_capture(unsigned int cpu_index, void *udata)
{
    emit_wfe(cpu_index, udata);
}
