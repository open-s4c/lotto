/*
 */
#ifndef LOTTO_QEMU_PLUGIN_UTIL_H
#define LOTTO_QEMU_PLUGIN_UTIL_H

// Qemu
#include <qemu-plugin.h>

#include <lotto/base/context.h>
// Qemu ARMCPU helper
#include <lotto/qlotto/qemu/armcpu.h>

void vcpu_mem_capture(unsigned int cpu_index, qemu_plugin_meminfo_t info,
                      uint64_t vaddr, void *udata);
void vcpu_insn_capture(unsigned int cpu_index, void *udata);
void vcpu_loop_capture(unsigned int cpu_index, void *udata);
void vcpu_event_capture(unsigned int cpu_index, void *udata);
void vcpu_exit_capture(unsigned int cpu_index, void *udata);

void vcpu_trace_start_capture(unsigned int cpu_index, void *udata);
void vcpu_trace_end_capture(unsigned int cpu_index, void *udata);
void vcpu_wfe_capture(unsigned int cpu_index, void *udata);
void vcpu_wfi_capture(unsigned int cpu_index, void *udata);

static inline void
register_exit_cb(context_t *ctx, struct qemu_plugin_insn *insn)
{
    qemu_plugin_register_vcpu_insn_exec_cb(insn, vcpu_exit_capture,
                                           QEMU_PLUGIN_CB_R_REGS, ctx);
}

static inline void
register_mem_cb(context_t *ctx, struct qemu_plugin_insn *insn)
{
    qemu_plugin_register_vcpu_mem_cb(
        insn, vcpu_mem_capture, QEMU_PLUGIN_CB_R_REGS, QEMU_PLUGIN_MEM_RW, ctx);
}

static inline void
register_insn_cb(context_t *ctx, struct qemu_plugin_insn *insn)
{
    qemu_plugin_register_vcpu_insn_exec_cb(insn, vcpu_insn_capture,
                                           QEMU_PLUGIN_CB_R_REGS, ctx);
}

static inline void
register_loop_cb(context_t *ctx, struct qemu_plugin_insn *insn)
{
    qemu_plugin_register_vcpu_insn_exec_cb(insn, vcpu_loop_capture,
                                           QEMU_PLUGIN_CB_R_REGS, ctx);
}

static inline void
register_event_cb(context_t *ctx, struct qemu_plugin_insn *insn)
{
    qemu_plugin_register_vcpu_insn_exec_cb(insn, vcpu_event_capture,
                                           QEMU_PLUGIN_CB_R_REGS, ctx);
}

static inline void
register_trace_start_cb(context_t *ctx, struct qemu_plugin_insn *insn)
{
    qemu_plugin_register_vcpu_insn_exec_cb(insn, vcpu_trace_start_capture,
                                           QEMU_PLUGIN_CB_R_REGS, ctx);
}

static inline void
register_trace_end_cb(context_t *ctx, struct qemu_plugin_insn *insn)
{
    qemu_plugin_register_vcpu_insn_exec_cb(insn, vcpu_trace_end_capture,
                                           QEMU_PLUGIN_CB_R_REGS, ctx);
}

static inline void
register_wfe_cb(context_t *ctx, struct qemu_plugin_insn *insn)
{
    qemu_plugin_register_vcpu_insn_exec_cb(insn, vcpu_wfe_capture,
                                           QEMU_PLUGIN_CB_R_REGS, ctx);
}

static inline void
register_wfi_cb(context_t *ctx, struct qemu_plugin_insn *insn)
{
    qemu_plugin_register_vcpu_insn_exec_cb(insn, vcpu_wfi_capture,
                                           QEMU_PLUGIN_CB_R_REGS, ctx);
}

// QLotto wrappers for Qemu plugins
#include <lotto/qlotto/qemu/stubs.h>


inline void icounter_init(icounter_t *cnt);
inline void icounter_free(icounter_t *cnt);
inline uint64_t icounter_get(icounter_t *cnt);

inline uint32_t get_instruction_data(const struct qemu_plugin_insn *insn);
inline void register_vcpu_insn_exec_inline(struct qemu_plugin_insn *insn,
                                           enum qemu_plugin_op op,
                                           icounter_t *ptr, uint64_t imm);
#endif // LOTTO_QEMU_PLUGIN_UTIL_H
