/**
 * @file events.h
 * @brief QEMU module semantic event identifiers.
 */
#ifndef LOTTO_MODULES_QEMU_EVENTS_H
#define LOTTO_MODULES_QEMU_EVENTS_H

#include <stddef.h>
#include <stdint.h>

#include <lotto/runtime/trap.h>

struct qemu_plugin_tb;

#define EVENT_QLOTTO_EXIT       196
#define EVENT_QEMU_INSTRUMENT   197
#define EVENT_QEMU_INIT         210
#define EVENT_QEMU_FINI         211
#define EVENT_QEMU_TRANSLATE    212
#define EVENT_QEMU_START        213
#define EVENT_QEMU_STOP         214
#define EVENT_QEMU_VCPU_INIT    215

typedef struct qemu_control_event {
    uintptr_t plugin_id;
    const void *info;
    int argc;
    char **argv;
} qemu_control_event_t;

typedef struct qemu_translate_event {
    uintptr_t plugin_id;
    struct qemu_plugin_tb *tb;
    size_t insn_count;
} qemu_translate_event_t;

typedef struct qemu_vcpu_init_event {
    unsigned int cpu_index;
    void *cpu;
    void *cpuenv;
    void *cpustatecc;
} qemu_vcpu_init_event_t;

#endif
