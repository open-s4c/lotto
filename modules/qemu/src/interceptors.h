#ifndef LOTTO_QEMU_INTERCEPTORS_H
#define LOTTO_QEMU_INTERCEPTORS_H

#include <qemu-plugin.h>
#include <stdbool.h>

void qemu_emit_init(qemu_plugin_id_t id, const qemu_info_t *info, int argc,
                    char **argv);
void qemu_emit_fini(void);
void qemu_emit_start(void);
void qemu_emit_stop(void);
void qemu_emit_vcpu_init(unsigned int cpu_index, void *cpu, void *cpuenv,
                         void *cpustatecc);
void qemu_set_instrumentation_enabled(unsigned int cpu_index, bool enabled);
bool qemu_instrumentation_enabled(unsigned int cpu_index);

#endif
