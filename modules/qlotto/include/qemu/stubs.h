/*
 */
#ifndef LOTTO_QEMU_PLUGIN_STUBS_H
#define LOTTO_QEMU_PLUGIN_STUBS_H

typedef qemu_plugin_u64 icounter_t;

static inline void
icounter_init(icounter_t *cnt)
{
    *cnt = qemu_plugin_scoreboard_u64(
        qemu_plugin_scoreboard_new(sizeof(uint64_t)));
}

static inline void
icounter_free(icounter_t *cnt)
{
    qemu_plugin_scoreboard_free(((qemu_plugin_u64)(*cnt)).score);
}

static inline uint64_t
icounter_get(icounter_t *cnt)
{
    return qemu_plugin_u64_sum((qemu_plugin_u64)(*cnt));
}

static inline uint32_t
get_instruction_data(const struct qemu_plugin_insn *insn)
{
    uint32_t opcode = 0;
    qemu_plugin_insn_data(insn, &opcode, sizeof(opcode));
    return opcode;
}

static inline void
register_vcpu_insn_exec_inline(struct qemu_plugin_insn *insn,
                               enum qemu_plugin_op op, icounter_t *ptr,
                               uint64_t imm)
{
    qemu_plugin_register_vcpu_insn_exec_inline_per_vcpu(
        insn, op, (qemu_plugin_u64)(*ptr), imm);
}

#endif /* LOTTO_QEMU_PLUGIN_STUBS_H */
