#include <stdint.h>
#include <string.h>

#include <lotto/qlotto/qemu/armcpu.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/ensure.h>

#define MAX_CPUS 1024

static struct gdb_arm_cpu {
    uint64_t num_cpus;
    uint64_t latest_cpu_index;
    int64_t cpu_index_offset;
    uint64_t current_core_id;
    // array for mapping cpu_index to CPUARMState*
    CPUARMState *gdb_parmcpu[MAX_CPUS];
    // array for mapping cpu_index to CPUState*
    void *gdb_pcpu[MAX_CPUS];
} _state;


CPUARMState *
gdb_get_parmcpu(uint64_t cpu_index)
{
    return _state.gdb_parmcpu[cpu_index];
}

void *
gdb_get_pcpu(uint64_t cpu_index)
{
    return _state.gdb_pcpu[cpu_index];
}

int64_t
gdb_get_cpu_offset(void)
{
    return _state.cpu_index_offset;
}

uint64_t
gdb_get_num_cpus(void)
{
    return _state.num_cpus;
}

uint64_t
gdb_get_latest_cpu_index(void)
{
    return _state.latest_cpu_index;
}

uint64_t
gdb_srv_get_cur_cpuindex()
{
    uint64_t cur_cpu = 0;
    if (_state.current_core_id == 0)
        cur_cpu = gdb_get_latest_cpu_index();
    else
        cur_cpu = _state.current_core_id - 1;
    return cur_cpu;
}

uint64_t
gdb_srv_get_cur_gdb_task()
{
    uint64_t cur_cpu = gdb_srv_get_cur_cpuindex();
    return cur_cpu + 1;
}

void
gdb_srv_set_current_core_id(int64_t core_id)
{
    ASSERT(core_id >= 0);
    _state.current_core_id = core_id;
}

static void
gdb_set_armcpu(unsigned int cpu_index, void *armcpu_v)
{
    // rl_fprintf(stderr, "Registered armcpu %u at %p\n", cpu_index, armcpu_v);

    _state.latest_cpu_index = cpu_index;
    if (cpu_index + 1 > _state.num_cpus)
        _state.num_cpus = cpu_index + 1;

    if (_state.gdb_parmcpu[cpu_index] == NULL)
        _state.gdb_parmcpu[cpu_index] = armcpu_v;

    ENSURE(_state.gdb_parmcpu[cpu_index] == armcpu_v);
}

static void
gdb_set_cpu(unsigned int cpu_index, void *cpu_v)
{
    // rl_fprintf(stderr, "Registered cpu %u at %p\n", cpu_index, cpu_v);
    _state.latest_cpu_index = cpu_index;
    if (cpu_index + 1 > _state.num_cpus)
        _state.num_cpus = cpu_index + 1;

    if (_state.gdb_pcpu[cpu_index] == NULL)
        _state.gdb_pcpu[cpu_index] = cpu_v;

    ENSURE(_state.gdb_pcpu[cpu_index] == cpu_v);
}

void
gdb_register_cpu(unsigned int cpu_index, void *cpu, void *armcpu)
{
    ENSURE(NULL != cpu);
    ENSURE(NULL != armcpu);

    gdb_set_armcpu(cpu_index, armcpu);
    gdb_set_cpu(cpu_index, cpu);
}

void
arm_cpu_init(void)
{
    _state.cpu_index_offset = -1;
}
