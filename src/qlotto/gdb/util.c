/*
 */

#include <stdint.h>

#include <lotto/qlotto/gdb/arm_cpu.h>
#include <lotto/qlotto/gdb/util.h>

uint64_t gdb_srv_vcpu_to_lotto_tid(uint64_t cpu_index);

uint64_t gdb_step_tick = 0;

static uint64_t
gdb_srv_gdb_vcpu_to_lotto_tid(uint64_t cpu_index)
{
    return gdb_get_cpu_offset() + cpu_index;
}

uint64_t
gdb_srv_lotto_tid_to_vcpu(uint64_t task_id)
{
    return task_id - gdb_get_cpu_offset();
}

uint64_t
gdb_srv_vcpu_to_gdb_tid(uint64_t cpu_index)
{
    return cpu_index + 1;
}

uint64_t
gdb_srv_gdb_tid_to_vcpu(uint64_t gdb_tid)
{
    return gdb_tid - 1;
}

uint64_t
gdb_srv_gdb_tid_to_lotto_tid(uint64_t gdb_tid)
{
    return gdb_srv_gdb_vcpu_to_lotto_tid(gdb_srv_gdb_tid_to_vcpu(gdb_tid));
}

uint64_t
gdb_srv_lotto_tid_to_gdb_tid(uint64_t task_id)
{
    return gdb_srv_vcpu_to_gdb_tid(gdb_srv_lotto_tid_to_vcpu(task_id));
}

void
gdb_tick(void)
{
    gdb_step_tick++;
}

uint64_t
gdb_get_tick(void)
{
    return gdb_step_tick;
}
