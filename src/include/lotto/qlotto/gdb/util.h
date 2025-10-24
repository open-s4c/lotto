/*
 */
#ifndef LOTTO_QEMU_PLUGIN_UTIL_H
#define LOTTO_QEMU_PLUGIN_UTIL_H

#include <stdint.h>

uint64_t gdb_srv_vcpu_to_lotto_tid(uint64_t cpu_index);
uint64_t gdb_srv_vcpu_to_gdb_tid(uint64_t cpu_index);

uint64_t gdb_srv_gdb_tid_to_vcpu(uint64_t gdb_tid);
uint64_t gdb_srv_gdb_tid_to_lotto_tid(uint64_t gdb_tid);

uint64_t gdb_srv_lotto_tid_to_gdb_tid(uint64_t task_id);
uint64_t gdb_srv_lotto_tid_to_vcpu(uint64_t task_id);

void gdb_tick(void);
uint64_t gdb_get_tick(void);

#endif // LOTTO_QEMU_PLUGIN_UTIL_H
