#include "interceptors.h"
#include <lotto/modules/qemu/util.h>
#include <lotto/yield.h>

uint64_t branch_back_count = 0;

void
emit_loop(unsigned int cpu_index, void *udata)
{
    uint64_t reduction = 1000;

    (void)udata;
    if (!qemu_instrumentation_enabled(cpu_index)) {
        return;
    }

    branch_back_count++;
    if (branch_back_count % reduction == 0) {
        lotto_yield(true);
    }
}
