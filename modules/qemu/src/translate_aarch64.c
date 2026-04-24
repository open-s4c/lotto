#include <glib.h>
#include <stdbool.h>
#include <string.h>

#include "translate_aarch64.h"
#include "interceptors.h"
#include <dice/chains/intercept.h>
#include <lotto/engine/pubsub.h>
#include <lotto/modules/qemu/events.h>
#include <lotto/modules/qemu/util.h>
#include <lotto/qemu/lotto_udf.h>
#include <lotto/runtime/capture_point.h>
#include <lotto/sys/stdlib.h>

#define AARCH64_WFE_OPCODE 0xd503205fU
#define AARCH64_WFI_OPCODE 0xd503207fU

static icounter_t g_inline_insn_count;
static bool g_inline_insn_count_initialized = false;

typedef struct qemu_translation_policy {
    bool memaccess;
    bool udf;
    bool yield;
    bool wf;
} qemu_translation_policy_t;

static bool
env_enabled(const char *name, bool default_value)
{
    const char *value = sys_getenv(name);
    if (value == NULL || value[0] == '\0') {
        return default_value;
    }
    return value[0] == '1';
}

static qemu_translation_policy_t
translation_policy(void)
{
    static bool initialized = false;
    static qemu_translation_policy_t cached;

    if (!initialized) {
        cached = (qemu_translation_policy_t){
            .memaccess = env_enabled("LOTTO_QEMU_INSTR_MEMACCESS", true),
            .udf       = env_enabled("LOTTO_QEMU_INSTR_UDF", true),
            .yield     = env_enabled("LOTTO_QEMU_INSTR_YIELD", true),
            .wf        = env_enabled("LOTTO_QEMU_INSTR_WF", true),
        };
        initialized = true;
    }

    return cached;
}

uint64_t
qemu_instruction_icount(void)
{
    if (!g_inline_insn_count_initialized) {
        return 0;
    }
    return icounter_get(&g_inline_insn_count);
}

static bool
mnemonic_has_prefix(const char *disas, const char *prefix)
{
    const size_t n = strlen(prefix);
    return strncmp(disas, prefix, n) == 0;
}

static bool
is_atomic_like_memaccess_insn(struct qemu_plugin_insn *insn)
{
    bool atomic = false;
    char *disas = qemu_plugin_insn_disas(insn);

    if (disas == NULL) {
        return false;
    }

    /* Acquire/release and exclusive families are treated as non-relaxed. */
    if (mnemonic_has_prefix(disas, "ldar") ||
        mnemonic_has_prefix(disas, "ldax") ||
        mnemonic_has_prefix(disas, "ldxr") ||
        mnemonic_has_prefix(disas, "ldapr") ||
        mnemonic_has_prefix(disas, "ldapur") ||
        mnemonic_has_prefix(disas, "ldlar") ||
        mnemonic_has_prefix(disas, "stlr") ||
        mnemonic_has_prefix(disas, "stlx") ||
        mnemonic_has_prefix(disas, "stxr") ||
        mnemonic_has_prefix(disas, "stxp") ||
        mnemonic_has_prefix(disas, "stllr") ||
        mnemonic_has_prefix(disas, "stlur") ||
        mnemonic_has_prefix(disas, "st64b")) {
        atomic = true;
    }

    g_free(disas);
    return atomic;
}

static void
bind_udf_instruction(struct qemu_plugin_insn *insn, uint32_t opcode,
                     const qemu_translation_policy_t *policy)
{
    if (!policy->udf || (opcode & UDF_MASK) != 0) {
        return;
    }
    (void)insn;
    (void)opcode;
}

static void
bind_wf_instruction(struct qemu_plugin_insn *insn, uint32_t opcode,
                    const qemu_translation_policy_t *policy)
{
    if (!policy->wf) {
        return;
    }

    if (opcode == AARCH64_WFE_OPCODE) {
        bind_wfe_callback(insn);
        return;
    }

    if (opcode == AARCH64_WFI_OPCODE) {
        bind_wfi_callback(insn);
    }
}

static void
decode_and_bind_insn(struct qemu_plugin_insn *insn,
                     const qemu_translation_policy_t *policy)
{
    uint32_t opcode = get_instruction_data(insn);
    register_vcpu_insn_exec_inline(insn, QEMU_PLUGIN_INLINE_ADD_U64,
                                   &g_inline_insn_count, 1);

    if (policy->memaccess) {
        bind_memaccess_callback(insn, is_atomic_like_memaccess_insn(insn));
    }

    if (policy->udf && opcode == LOTTO_TRAP_A64_VAL) {
        bind_udf_trap_callback(insn);
        return;
    }

    bind_wf_instruction(insn, opcode, policy);
    bind_udf_instruction(insn, opcode, policy);
}

void
qemu_on_tb_translate(qemu_plugin_id_t id, struct qemu_plugin_tb *tb)
{
    (void)id;
    qemu_translation_policy_t policy = translation_policy();
    size_t n                         = qemu_plugin_tb_n_insns(tb);
    for (size_t i = 0; i < n; i++) {
        struct qemu_plugin_insn *insn = qemu_plugin_tb_get_insn(tb, i);
        decode_and_bind_insn(insn, &policy);
    }
}

void
qemu_dispatch_tb_translate(qemu_plugin_id_t id, struct qemu_plugin_tb *tb)
{
    qemu_translate_event_t ev = {
        .plugin_id  = (uintptr_t)id,
        .tb         = tb,
        .insn_count = qemu_plugin_tb_n_insns(tb),
    };

    qemu_on_tb_translate(id, tb);
    PS_PUBLISH(CHAIN_QEMU_CONTROL, EVENT_QEMU_TRANSLATE, &ev, 0);
}

void
qemu_on_plugin_start(qemu_plugin_id_t id, const qemu_info_t *info, int argc,
                     char **argv)
{
    (void)id;
    (void)info;
    (void)argc;
    (void)argv;
    icounter_init(&g_inline_insn_count);
    g_inline_insn_count_initialized = true;
    qemu_emit_start();
}

void
qemu_on_plugin_stop(void)
{
    if (g_inline_insn_count_initialized) {
        icounter_free(&g_inline_insn_count);
        g_inline_insn_count_initialized = false;
    }
    qemu_emit_stop();
}
