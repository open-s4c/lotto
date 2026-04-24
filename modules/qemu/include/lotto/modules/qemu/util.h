/**
 * @file util.h
 * @brief QEMU module declarations for util.
 */
#ifndef LOTTO_QEMU_PLUGIN_UTIL_H
#define LOTTO_QEMU_PLUGIN_UTIL_H

#include <stdint.h>
// Qemu
#include <qemu-plugin.h>

#include <lotto/runtime/capture_point.h>
// Qemu ARMCPU helper
#include <lotto/modules/qemu/armcpu.h>

CPUARMState *qemu_get_armcpu_context(unsigned int cpu_index);

void vcpu_mem_capture(unsigned int cpu_index, qemu_plugin_meminfo_t info,
                      uint64_t vaddr, void *udata);
void vcpu_insn_capture(unsigned int cpu_index, void *udata);
void vcpu_loop_capture(unsigned int cpu_index, void *udata);
void vcpu_event_capture(unsigned int cpu_index, void *udata);
void emit_udf_trap(unsigned int cpu_index, void *udata);
void emit_wfe(unsigned int cpu_index, void *udata);
void emit_wfi(unsigned int cpu_index, void *udata);

void vcpu_wfe_capture(unsigned int cpu_index, void *udata);
void vcpu_wfi_capture(unsigned int cpu_index, void *udata);

static inline void
bind_udf_trap_callback(struct qemu_plugin_insn *insn)
{
    qemu_plugin_register_vcpu_insn_exec_cb(insn, emit_udf_trap,
                                           QEMU_PLUGIN_CB_R_REGS, NULL);
}

static inline void
bind_wfe_callback(struct qemu_plugin_insn *insn)
{
    qemu_plugin_register_vcpu_insn_exec_cb(insn, emit_wfe,
                                           QEMU_PLUGIN_CB_NO_REGS, NULL);
}

static inline void
bind_wfi_callback(struct qemu_plugin_insn *insn)
{
    qemu_plugin_register_vcpu_insn_exec_cb(insn, emit_wfi,
                                           QEMU_PLUGIN_CB_NO_REGS, NULL);
}

static inline void
register_mem_cb(capture_point *cp, struct qemu_plugin_insn *insn)
{
    qemu_plugin_register_vcpu_mem_cb(
        insn, vcpu_mem_capture, QEMU_PLUGIN_CB_R_REGS, QEMU_PLUGIN_MEM_RW, cp);
}

void emit_memaccess(unsigned int cpu_index, qemu_plugin_meminfo_t info,
                    uint64_t vaddr, void *udata);
void emit_ma_read(capture_point *cp, uint64_t vaddr, uint64_t size);
void emit_ma_write(capture_point *cp, uint64_t vaddr, uint64_t size);
void emit_ma_aread(capture_point *cp, uint64_t vaddr, uint64_t size);
void emit_ma_awrite(capture_point *cp, uint64_t vaddr, uint64_t size);
void emit_ma_rmw(capture_point *cp, uint64_t vaddr, uint64_t size);
void emit_ma_xchg(capture_point *cp, uint64_t vaddr, uint64_t size);
void emit_ma_cmpxchg(capture_point *cp, uint64_t vaddr, uint64_t size);

static inline void
bind_memaccess_callback(struct qemu_plugin_insn *insn, bool is_atomic_like)
{
    qemu_plugin_register_vcpu_mem_cb(insn, emit_memaccess,
                                     QEMU_PLUGIN_CB_R_REGS, QEMU_PLUGIN_MEM_RW,
                                     (void *)(uintptr_t)is_atomic_like);
}

static inline void
register_insn_cb(capture_point *cp, struct qemu_plugin_insn *insn)
{
    qemu_plugin_register_vcpu_insn_exec_cb(insn, vcpu_insn_capture,
                                           QEMU_PLUGIN_CB_R_REGS, cp);
}

static inline void
register_wfe_cb(capture_point *cp, struct qemu_plugin_insn *insn)
{
    qemu_plugin_register_vcpu_insn_exec_cb(insn, vcpu_wfe_capture,
                                           QEMU_PLUGIN_CB_R_REGS, cp);
}

static inline void
register_wfi_cb(capture_point *cp, struct qemu_plugin_insn *insn)
{
    qemu_plugin_register_vcpu_insn_exec_cb(insn, vcpu_wfi_capture,
                                           QEMU_PLUGIN_CB_R_REGS, cp);
}

#include <lotto/modules/qemu/stubs.h>


inline void icounter_init(icounter_t *cnt);
inline void icounter_free(icounter_t *cnt);
inline uint64_t icounter_get(icounter_t *cnt);

inline uint32_t get_instruction_data(const struct qemu_plugin_insn *insn);
inline void register_vcpu_insn_exec_inline(struct qemu_plugin_insn *insn,
                                           enum qemu_plugin_op op,
                                           icounter_t *ptr, uint64_t imm);
#endif // LOTTO_QEMU_PLUGIN_UTIL_H
