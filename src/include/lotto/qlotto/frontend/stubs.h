#ifndef LOTTO_QLOTTO_STUBS_H
#define LOTTO_QLOTTO_STUBS_H

#include <qemu-plugin.h>
#include <stdint.h>

#include <lotto/qlotto/gdb/handling/stop_reason.h>
#include <lotto/util/macros.h>

WEAK void gdb_set_stop_signal(int32_t signum);
WEAK void gdb_set_stop_reason(stop_reason_t signum, uint64_t n);
WEAK void gdb_handle_mem_capture(unsigned int cpu_index,
                                 qemu_plugin_meminfo_t info, uint64_t vaddr,
                                 void *udata);

#endif // LOTTO_QLOTTO_STUBS_H
