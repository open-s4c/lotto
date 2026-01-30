#ifndef LOTTO_GDB_INTERCEPTOR_H
#define LOTTO_GDB_INTERCEPTOR_H

#include <qemu-plugin.h>
#include <stdint.h>

void gdb_capture(unsigned int cpu_index, void *udata);
void gdb_handle_mem_capture(unsigned int cpu_index, qemu_plugin_meminfo_t info,
                            uint64_t vaddr, void *udata);

#endif // LOTTO_GDB_INTERCEPTOR_H
