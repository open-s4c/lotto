#include <qemu-plugin.h>
#include <stdbool.h>
#include <stdint.h>

#include <lotto/engine/sequencer.h>
#include <lotto/qlotto/gdb/arm_cpu.h>
#include <lotto/qlotto/gdb/halter.h>
#include <lotto/qlotto/gdb/handling/break.h>
#include <lotto/qlotto/gdb/handling/stop_reason.h>
#include <lotto/qlotto/gdb/util.h>
#include <lotto/unsafe/_sys.h>

// qemu
#include <qemu-plugin.h>

bool qemu_gdb_enabled(void);

void
gdb_handle_mem_capture(unsigned int cpu_index, qemu_plugin_meminfo_t info,
                       uint64_t vaddr, void *udata)
{
    if (!qemu_gdb_enabled())
        return;

    bool is_write = qemu_plugin_mem_is_store(info);

    if (gdb_has_watchpoint(vaddr, is_write)) {
        gdb_set_stop_signal(5);
        gdb_set_stop_reason(STOP_REASON_awatch, vaddr);
        gdb_srv_set_current_core_id(CAST_TYPE(int64_t, cpu_index + 1));
        gdb_execution_halt(gdb_srv_vcpu_to_gdb_tid(cpu_index));
    }
}

bool qemu_gdb_get_wait(void);
void qemu_gdb_set_wait(bool wait);

void
gdb_capture(unsigned int cpu_index, void *udata)
{
    gdb_tick();
    if (!qemu_gdb_enabled())
        return;
    CPUARMState *armcpu = gdb_get_parmcpu(cpu_index);
    uint64_t pc         = CAST_TYPE(uint64_t, (uintptr_t)udata);

    if (qemu_gdb_get_wait()) {
        rl_fprintf(stderr, "Waiting for gdb to continue\n");
        uint64_t old_pc = armcpu->pc;
        armcpu->pc      = pc;
        gdb_set_stop_signal(5);
        gdb_set_stop_reason(STOP_REASON_NONE, 0);
        gdb_srv_set_current_core_id(CAST_TYPE(int64_t, cpu_index + 1));
        gdb_execution_halt(gdb_srv_vcpu_to_gdb_tid(cpu_index));
        armcpu->pc = old_pc;
        return;
    }

    if (gdb_get_break_next() == (int32_t)cpu_index ||
        gdb_get_break_next() == GDB_BREAK_ANY) {
        // fprintf(stderr, "Hit break next 0x%lx on cpu_index %u (tick %lu)\n",
        // pc, cpu_index, gdb_get_tick());
        gdb_set_break_next(GDB_BREAK_STEP_OFF);
        uint64_t old_pc = armcpu->pc;
        armcpu->pc      = pc;
        gdb_set_stop_signal(5);
        gdb_set_stop_reason(STOP_REASON_NONE, 0);
        gdb_srv_set_current_core_id(CAST_TYPE(int64_t, cpu_index + 1));
        gdb_execution_halt(gdb_srv_vcpu_to_gdb_tid(cpu_index));
        armcpu->pc = old_pc;
        return;
    }

    if (gdb_has_breakpoint(pc)) {
        // fprintf(stderr, "Hit breakpoint 0x%lx on cpu_index %u\n", pc,
        // cpu_index);
        uint64_t old_pc = armcpu->pc;
        armcpu->pc      = pc;
        gdb_set_stop_signal(5);
        gdb_set_stop_reason(STOP_REASON_swbreak, 0);
        gdb_srv_set_current_core_id(CAST_TYPE(int64_t, cpu_index + 1));
        gdb_execution_halt(gdb_srv_vcpu_to_gdb_tid(cpu_index));
        armcpu->pc = old_pc;
        return;
    }

    if (gdb_is_outside_breakpoint_range(pc, CAST_TYPE(int32_t, cpu_index))) {
        // there is a break point range to be left and break after, we are not
        // inside it
        uint64_t old_pc = armcpu->pc;
        armcpu->pc      = pc;
        gdb_set_stop_signal(5);
        gdb_set_stop_reason(STOP_REASON_swbreak, 0);
        gdb_srv_set_current_core_id(CAST_TYPE(int64_t, cpu_index + 1));
        gdb_del_breakpoint_range_exclude(pc);
        gdb_execution_halt(gdb_srv_vcpu_to_gdb_tid(cpu_index));
        armcpu->pc = old_pc;
        return;
    }

    if (gdb_has_breakpoint_gdb_clock(gdb_get_tick()) ||
        gdb_has_breakpoint_lotto_clock(sequencer_get_clk())) {
        // fprintf(stderr, "Hit clock breakpoint on cpu_index %u\n", cpu_index);
        uint64_t old_pc = armcpu->pc;
        armcpu->pc      = pc;
        gdb_set_stop_signal(5);
        gdb_set_stop_reason(STOP_REASON_NONE, 0);
        gdb_srv_set_current_core_id(CAST_TYPE(int64_t, cpu_index + 1));
        gdb_execution_halt(gdb_srv_vcpu_to_gdb_tid(cpu_index));
        armcpu->pc = old_pc;
        return;
    }
}
