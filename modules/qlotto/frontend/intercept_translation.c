/*
 */

#include <qemu-plugin.h>
#include <stdbool.h>

#include <lotto/base/context.h>
#include <lotto/base/reason.h>
#include <lotto/perf/perf.h>
#include <lotto/qemu/lotto_udf.h>
#include <lotto/qlotto/frontend/event.h>
#include <lotto/qlotto/frontend/intercept_translation.h>
#include <lotto/qlotto/frontend/interceptor.h>
#include <lotto/qlotto/mapping.h>
#include <lotto/runtime/intercept.h>
#include <lotto/sys/ensure.h>
#include <lotto/sys/stdlib.h>


// capstone
#include <capstone/arm64.h>
#include <capstone/capstone.h>

typedef struct instrumentation_state_s {
    csh disasm;
    map_t events;
    bool skip_loop_instrumentation;
} instrumentation_state_t;

static instrumentation_state_t _state;

void
udf_decode_reg(context_t *ctx, struct qemu_plugin_insn *insn)
{
    uint32_t opcode = get_instruction_data(insn);

    ASSERT(0 == (opcode & UDF_MASK));
    switch (opcode) {
        case LOTTO_YIELD_A64_VAL:
            ctx->cat = CAT_SYS_YIELD;
            register_insn_cb(ctx, insn);
            break;
        case LOTTO_QEMUQUIT_A64_VAL:
        case LOTTO_TEST_SUCCESS_VAL:
            ctx->cat               = CAT_NONE;
            ctx->args[0].value.u32 = REASON_SUCCESS;
            register_exit_cb(ctx, insn);
            break;
        case LOTTO_TEST_FAIL_VAL:
            ctx->cat               = CAT_NONE;
            ctx->args[0].value.u32 = REASON_ASSERT_FAIL;
            register_exit_cb(ctx, insn);
            break;

        // Guest lock actions
        case LOTTO_LOCK_ACQ_A64_VAL:
            ctx->cat = CAT_RSRC_ACQUIRING;
            register_insn_cb(ctx, insn);
            break;
        case LOTTO_LOCK_REL_A64_VAL:
            ctx->cat = CAT_RSRC_RELEASED;
            register_insn_cb(ctx, insn);
            break;
        case LOTTO_LOCK_TRYACQ_A64_VAL:
        case LOTTO_LOCK_TRYACQ_SUCC_A64_VAL:
        case LOTTO_LOCK_TRYACQ_FAIL_A64_VAL:
            break;

        // Lotto Region
        case LOTTO_REGION_IN_VAL:
            ctx->cat = CAT_REGION_IN;
            register_insn_cb(ctx, insn);
            break;
        case LOTTO_REGION_OUT_VAL:
            ctx->cat = CAT_REGION_OUT;
            register_insn_cb(ctx, insn);
            break;

        case LOTTO_NO_INSTRUMENT_VAL:
            _state.skip_loop_instrumentation = true;
            break;

        case LOTTO_TRACE_START_VAL:
            ctx->cat = CAT_NONE;
            register_trace_start_cb(ctx, insn);
            break;
        case LOTTO_TRACE_END_VAL:
            ctx->cat = CAT_NONE;
            register_trace_end_cb(ctx, insn);
            break;

        default:
            break;
    }
    return;
}

void
loop_check(int32_t opcount, int32_t opnum, uint64_t pc, context_t *ctx,
           cs_insn *insn_cs, struct qemu_plugin_insn *insn)
{
    int64_t b_insn_diff = 0;

    ASSERT(opcount == insn_cs->detail->arm64.op_count);
    ASSERT(ARM64_OP_IMM == insn_cs->detail->arm64.operands[opnum].type);
    if (_state.skip_loop_instrumentation) {
        _state.skip_loop_instrumentation = false;
        return;
    }

    b_insn_diff =
        (insn_cs->detail->arm64.operands[opnum].imm - (int64_t)pc) / 4;

    if (b_insn_diff <= 0) {
        ctx->cat  = CAT_SYS_YIELD;
        ctx->func = "forced event by short backedge";
        if (INSTR_LOOP)
            register_loop_cb(ctx, insn);
    }
}

void
set_ctx_by_insn(context_t *ctx, cs_insn *insn_cs, struct qemu_plugin_insn *insn,
                uint64_t pc)
{
    ctx->cat  = mapping_arm64[insn_cs->id];
    ctx->func = mapping_cat[ctx->cat].func;

    if (mapping_cat[ctx->cat].cb != NULL) {
        int cat_tmp = ctx->cat;
        if (cat_tmp > CAT_END_) {
            ctx->cat = cat_extra_mapping[ctx->cat];
        }
        mapping_cat[cat_tmp].cb(ctx, insn);
        return;
    }

    switch (insn_cs->id) {
        case ARM64_INS_CBZ:
        case ARM64_INS_CBNZ:
            loop_check(2, 1, pc, ctx, insn_cs, insn);
            break;
        case ARM64_INS_TBL:
        case ARM64_INS_TBNZ:
        case ARM64_INS_TBX:
        case ARM64_INS_TBZ:
            loop_check(3, 2, pc, ctx, insn_cs, insn);
            break;
        case ARM64_INS_B:
        case ARM64_INS_BC:
            loop_check(1, 0, pc, ctx, insn_cs, insn);
            break;

        case ARM64_INS_BRAA:
        case ARM64_INS_BRAAZ:
        case ARM64_INS_BRAB:
        case ARM64_INS_BRABZ:
        case ARM64_INS_BLRAA:
        case ARM64_INS_BLRAAZ:
        case ARM64_INS_BLRAB:
        case ARM64_INS_BLRABZ:
            ASSERT(0);
            break;
        case ARM64_INS_WFE:
            register_vcpu_insn_exec_inline(insn, QEMU_PLUGIN_INLINE_ADD_U64,
                                           get_instruction_counter_wfe(), 1);
            register_wfe_cb(ctx, insn);
            break;
        case ARM64_INS_WFI:
            register_vcpu_insn_exec_inline(insn, QEMU_PLUGIN_INLINE_ADD_U64,
                                           get_instruction_counter_wfi(), 1);
            register_wfi_cb(ctx, insn);
            break;
        default:
            break;
    }
}

void
do_disasm_reg(struct qemu_plugin_insn *insn)
{
    uint32_t opcode = get_instruction_data(insn);
    uint64_t pc     = qemu_plugin_insn_vaddr(insn);
    eventi_t *event = NULL;

    cs_insn *insn_cs;
    if (cs_disasm(_state.disasm, (const unsigned char *)&opcode, sizeof(opcode),
                  pc, 0, &insn_cs) == 1) {
        context_t *ctx;
        ctx     = (context_t *)sys_malloc(sizeof(context_t));
        ctx->pc = pc;

        // Set context depending on actual instruction
        set_ctx_by_insn(ctx, insn_cs, insn, pc);
    }

    event = qlotto_get_event(&_state.events, pc);
    if (event != NULL) {
        context_t *ctx = (context_t *)sys_malloc(sizeof(context_t));
        ctx->cat       = event->cat;
        // Inform Lotto about guest function name
        ctx->func = event->func_name;
        ctx->pc   = pc;
        register_event_cb(ctx, insn);
    }
}

void
vcpu_tb_trans(qemu_plugin_id_t id, struct qemu_plugin_tb *tb)
{
    task_id tid = get_task_id();
    __perf_transition_qemu(tid, STATE_Q_QEMU, STATE_Q_PLUGIN, false);

    size_t n = qemu_plugin_tb_n_insns(tb);

    for (size_t i = 0; i < n; i++) {
        struct qemu_plugin_insn *insn = qemu_plugin_tb_get_insn(tb, i);

        /*
            Add inline counter of executed instructions
            Used for deterministic time progression in lotto
        */

        register_vcpu_insn_exec_inline(insn, QEMU_PLUGIN_INLINE_ADD_U64,
                                       get_instruction_counter(), 1);

        /*
            Disassemble and check actual instruction
            Fill context and add appropriate callback
        */

        do_disasm_reg(insn);
    }
    __perf_transition_qemu(tid, STATE_Q_PLUGIN, STATE_Q_QEMU, false);
}

void
instrumentation_init()
{
    task_id tid = get_task_id();
    __perf_transition_qemu(tid, STATE_Q_QEMU, STATE_Q_PLUGIN, false);

    ENSURE(cs_open(CS_ARCH_ARM64, CS_MODE_ARM, &_state.disasm) == CS_ERR_OK);
    ENSURE(cs_option(_state.disasm, CS_OPT_DETAIL, CS_OPT_ON) == CS_ERR_OK);

    map_init(&_state.events, MARSHABLE_STATIC(sizeof(eventi_t)));

    _state.skip_loop_instrumentation = false;

    frontend_init();
    __perf_transition_qemu(tid, STATE_Q_PLUGIN, STATE_Q_QEMU, false);
}

void
instrumentation_exit(void)
{
    frontend_exit();
}
