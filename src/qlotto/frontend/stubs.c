
#include <lotto/qlotto/frontend/stubs.h>
#include <lotto/sys/logger.h>
#include <lotto/util/macros.h>

WEAK void
gdb_set_stop_signal(int32_t signum)
{
    return;
}

WEAK void
gdb_set_stop_reason(stop_reason_t sr, uint64_t n)
{
    return;
}

WEAK void
gdb_handle_mem_capture(unsigned int cpu_index, qemu_plugin_meminfo_t info,
                       uint64_t vaddr, void *udata)
{
    return;
}
