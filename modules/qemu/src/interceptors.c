#include <stddef.h>

#include "interceptors.h"
#include "translate_aarch64.h"
#include <dice/pubsub.h>
#include <lotto/engine/pubsub.h>
#include <lotto/modules/qemu/callbacks.h>
#include <lotto/modules/qemu/events.h>
#include <lotto/sys/assert.h>

#define QEMU_MAX_VCPUS 1024

static bool g_qemu_instrumentation_enabled[QEMU_MAX_VCPUS];
static qemu_control_event_t g_qemu_control_event;
static bool g_qemu_init_emitted = false;
static bool g_qemu_stop_emitted = false;
static bool g_qemu_fini_emitted = false;

REGISTER_GLOBAL_DEFINE(qemu_register_cpu_context, void, unsigned int cpu_index,
                       void *cpu, void *cpuenv, void *cpustatecc)

void
qemu_set_instrumentation_enabled(unsigned int cpu_index, bool enabled)
{
    ASSERT(cpu_index < QEMU_MAX_VCPUS);
    g_qemu_instrumentation_enabled[cpu_index] = enabled;
}

bool
qemu_instrumentation_enabled(unsigned int cpu_index)
{
    ASSERT(cpu_index < QEMU_MAX_VCPUS);
    return g_qemu_instrumentation_enabled[cpu_index];
}

void
qemu_emit_init(qemu_plugin_id_t id, const qemu_info_t *info, int argc,
               char **argv)
{
    for (size_t i = 0; i < QEMU_MAX_VCPUS; i++) {
        g_qemu_instrumentation_enabled[i] = true;
    }
    g_qemu_control_event = (qemu_control_event_t){
        .plugin_id = (uintptr_t)id,
        .info      = info,
        .argc      = argc,
        .argv      = argv,
    };
    g_qemu_init_emitted = true;
    g_qemu_stop_emitted = false;
    g_qemu_fini_emitted = false;
    PS_PUBLISH(CHAIN_QEMU_CONTROL, EVENT_QEMU_INIT, &g_qemu_control_event, 0);
}

void
qemu_emit_start(void)
{
    if (!g_qemu_init_emitted) {
        return;
    }
    PS_PUBLISH(CHAIN_QEMU_CONTROL, EVENT_QEMU_START, &g_qemu_control_event, 0);
}

void
qemu_emit_stop(void)
{
    if (!g_qemu_init_emitted || g_qemu_stop_emitted) {
        return;
    }
    g_qemu_stop_emitted = true;
    PS_PUBLISH(CHAIN_QEMU_CONTROL, EVENT_QEMU_STOP, &g_qemu_control_event, 0);
}

void
qemu_emit_vcpu_init(unsigned int cpu_index, void *cpu, void *cpuenv,
                    void *cpustatecc)
{
    qemu_vcpu_init_event_t ev = {
        .cpu_index  = cpu_index,
        .cpu        = cpu,
        .cpuenv     = cpuenv,
        .cpustatecc = cpustatecc,
    };
    PS_PUBLISH(CHAIN_QEMU_CONTROL, EVENT_QEMU_VCPU_INIT, &ev, 0);
}

void
qemu_emit_fini(void)
{
    if (!g_qemu_init_emitted || g_qemu_fini_emitted) {
        return;
    }
    g_qemu_fini_emitted = true;
    PS_PUBLISH(CHAIN_QEMU_CONTROL, EVENT_QEMU_FINI, &g_qemu_control_event, 0);
}

ON_FINALIZATION_PHASE({
    qemu_on_plugin_stop();
    qemu_emit_fini();
})
