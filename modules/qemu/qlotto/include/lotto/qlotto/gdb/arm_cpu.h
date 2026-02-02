#ifndef LOTTO_ARM_CPU_H
#define LOTTO_ARM_CPU_H

#include <stdint.h>

#include <lotto/qlotto/qemu/armcpu.h>

void arm_cpu_init(void);

uint64_t gdb_srv_get_cur_cpuindex();
void gdb_srv_set_current_core_id(int64_t core_id);
uint64_t gdb_get_num_cpus(void);
uint64_t gdb_srv_get_cur_gdb_task();

int64_t gdb_get_cpu_offset(void);

CPUARMState *gdb_get_parmcpu(uint64_t cpu_index);
void *gdb_get_pcpu(uint64_t cpu_index);

#endif // LOTTO_ARM_CPU_H
