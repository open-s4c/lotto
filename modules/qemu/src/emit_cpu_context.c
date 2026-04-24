#include "interceptors.h"
#include <lotto/modules/qemu/armcpu.h>
#include <lotto/modules/qemu/util.h>

#define MAX_CPUS 1024

static CPUARMState *g_armcpu[MAX_CPUS];

void
qemu_register_cpu_context(unsigned int cpu_index, void *cpu, void *armcpu,
                          void *cpustatecc)
{
    (void)cpu;
    (void)cpustatecc;
    if (cpu_index < MAX_CPUS) {
        g_armcpu[cpu_index] = (CPUARMState *)armcpu;
        qemu_emit_vcpu_init(cpu_index, cpu, armcpu, cpustatecc);
    }
}

CPUARMState *
qemu_get_armcpu_context(unsigned int cpu_index)
{
    if (cpu_index >= MAX_CPUS) {
        return NULL;
    }
    return g_armcpu[cpu_index];
}
