/*
 */

#include <signal.h>
#include <unistd.h>

#include <lotto/base/cappt.h>
#include <lotto/base/context.h>
#include <lotto/base/reason.h>
#include <lotto/base/task_id.h>
#include <lotto/engine/dispatcher.h>
#include <lotto/engine/prng.h>
#include <lotto/perf/perf.h>
#include <lotto/qlotto/frontend/event.h>
#include <lotto/qlotto/frontend/perf.h>
#include <lotto/qlotto/frontend/stubs.h>
#include <lotto/qlotto/gdb/gdb_server_stub.h>
#include <lotto/qlotto/gdb/handling/stop_reason.h>
#include <lotto/qlotto/mapping.h>
#include <lotto/qlotto/qemu/cpustatecc.h>
#include <lotto/qlotto/qemu/util.h>
#include <lotto/runtime/intercept.h>
#include <lotto/runtime/runtime.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/ensure.h>
#include <lotto/sys/memory.h>

// capstone
#include <capstone/arm64.h>
#include <capstone/capstone.h>

#define MAX_CPUS 1024
typedef struct frontend_state_s {
    uint64_t inst_loop_count;
    int32_t cpu_index_offset;
    CPUARMState *qlotto_parmcpu[MAX_CPUS];
    void *qlotto_pcpu[MAX_CPUS];
    CPUStateCC *qlotto_pcpustatecc[MAX_CPUS];
    uint64_t cpustate_cc_offset;
    icounter_t inline_insn_count;
    uint64_t sample_counter;
    icounter_t inline_insn_count_wfe;
    icounter_t inline_insn_count_wfi;
} frontend_state_t;

static frontend_state_t frontend_state;

icounter_t *
get_instruction_counter(void)
{
    return &frontend_state.inline_insn_count;
}

icounter_t *
get_instruction_counter_wfi(void)
{
    return &frontend_state.inline_insn_count_wfi;
}

icounter_t *
get_instruction_counter_wfe(void)
{
    return &frontend_state.inline_insn_count_wfe;
}

/*******************************************************************************
 * plugin_lotto region handler
 ******************************************************************************/
static int _in_fine_region[MAX_TASKS];

static void
_fine_capture(const context_t *ctx)
{
    ASSERT(_in_fine_region[ctx->id] >= 0);
    _in_fine_region[ctx->id]++;
}

static void
_coarse_capture(const context_t *ctx)
{
    if (_in_fine_region[ctx->id] > 0)
        _in_fine_region[ctx->id]--;
}


static bool
_should_drop(const context_t *ctx, bool inc)
{
    if (inc) {
        frontend_state.sample_counter = lcg_next(frontend_state.sample_counter);
    }

    if (_in_fine_region[ctx->id])
        return false;

    if (mapping_cat[ctx->cat].sample == 0)
        return false;

    if (frontend_state.sample_counter % mapping_cat[ctx->cat].sample == 0)
        return false;

    return true;
}

static void
_handler(const context_t *ctx, event_t *e)
{
    switch (ctx->cat) {
        case CAT_REGION_IN:
            _fine_capture(ctx);
            break;
        case CAT_REGION_OUT:
            _coarse_capture(ctx);
            break;
        default:
            if (_should_drop(ctx, false))
                e->skip = true;
            break;
    }
}
REGISTER_HANDLER_EXTERNAL(SLOT_USER_FILTER, _handler);

void
qlotto_set_armcpu(unsigned int cpu_index, void *armcpu_v)
{
    if (frontend_state.qlotto_parmcpu[cpu_index] == NULL)
        frontend_state.qlotto_parmcpu[cpu_index] = armcpu_v;

    ASSERT(frontend_state.qlotto_parmcpu[cpu_index] == armcpu_v);
}

static void
qlotto_set_cpu(unsigned int cpu_index, void *cpu_v)
{
    if (frontend_state.qlotto_pcpu[cpu_index] == NULL)
        frontend_state.qlotto_pcpu[cpu_index] = cpu_v;

    ASSERT(frontend_state.qlotto_pcpu[cpu_index] == cpu_v);
}

void
qlotto_set_cpustatecc(unsigned int cpu_index, void *cpustatecc_v)
{
    if (frontend_state.qlotto_pcpustatecc[cpu_index] == NULL)
        frontend_state.qlotto_pcpustatecc[cpu_index] = cpustatecc_v;

    ASSERT(frontend_state.qlotto_pcpustatecc[cpu_index] == cpustatecc_v);
}

void
qlotto_register_cpu(unsigned int cpu_index, void *cpu, void *armcpu,
                    void *cpustatecc_v)
{
    ENSURE(NULL != cpu);
    ENSURE(NULL != armcpu);

    if (frontend_state.cpu_index_offset < 0) {
        frontend_state.cpu_index_offset =
            CAST_TYPE(int32_t, (get_task_id() - cpu_index));
    }

    qlotto_set_cpu(cpu_index, cpu);
    qlotto_set_armcpu(cpu_index, armcpu);
    qlotto_set_cpustatecc(cpu_index, cpustatecc_v);
    if (frontend_state.cpustate_cc_offset == 0)
        frontend_state.cpustate_cc_offset = cpustatecc_v - cpu;
}

CPUARMState *
qlotto_get_armcpu(unsigned int cpu_index)
{
    ENSURE(cpu_index <= MAX_CPUS);
    ENSURE(NULL != frontend_state.qlotto_parmcpu[cpu_index]);
    return frontend_state.qlotto_parmcpu[cpu_index];
}

/*
 * qemu vcpu to lotto tid offset
 */
static inline int32_t
get_qemu_thread_offset()
{
    ENSURE(frontend_state.cpu_index_offset > 0);
    return frontend_state.cpu_index_offset;
}

static inline uint64_t
vcpu_to_lotto_tid(uint32_t cpu_index)
{
    return get_qemu_thread_offset() + cpu_index;
}

static inline uint32_t __attribute__((unused))
lotto_tid_to_vcpu(uint64_t task_id)
{
    return (uint32_t)(task_id - get_qemu_thread_offset());
}

static inline uint64_t
get_cpu_vid(CPUARMState *armcpu)
{
    uint64_t vid = 0;
    if (((armcpu->pstate >> 2) & 0x3) == 0) {
        // user space
        vid = armcpu->cp15.tpidr_el[0];
    } else {
        // kernel space
        vid = armcpu->cp15.tpidr_el[1];
    }
    return vid == NO_TASK ? ANY_TASK : vid;
}

static void
_set_ctx_id(context_t *ctx, unsigned int cpu_index)
{
    uint64_t tid = get_task_id();

    if (frontend_state.cpu_index_offset < 0) {
        frontend_state.cpu_index_offset = CAST_TYPE(int32_t, (tid - cpu_index));
    }
    ENSURE(vcpu_to_lotto_tid(cpu_index) == tid);

    CPUARMState *armcpu = qlotto_get_armcpu(cpu_index);

    uint64_t vid = get_cpu_vid(armcpu);

    // pstate is used to detect if guest OS is in kernel or user space
    ctx->pstate = armcpu->pstate;

    // If there is a thread running, then vid_el0 has the pointer to the TLS.
    // We use this pointer as virtual task id.
    ctx->vid = vid;

    // Task id is the cpu_index plus the offset caused by the threads of QEMU
    // which are also counted as tasks. +1 as tasks start with 1 and cpu_index
    // starts with 0 +2 as there are two qemu threads before the vcpu threads
    // are started
    ctx->id = tid;
}

// pass fname == NULL to terminate Lotto
static void
_intercept(context_t *ctx, const char *fname)
{
    // TODO: it seems that ctx is not being really reset, and sometimes
    // ctx.func contains garbage.
    //       To avoid SEGFAULTs, we always set it here.
    if (NULL == ctx->func)
        ctx->func = fname;

    // Inform Lotto how much time has elapsed
    ctx->icount = icounter_get(&frontend_state.inline_insn_count);

    if (fname)
        intercept_capture(ctx);
    else {
        if (ctx->args[0].value.u32 == REASON_SUCCESS) {
            lotto_exit(ctx, ctx->args[0].value.u32);
        }
        if (ctx->args[0].value.u32 == REASON_ASSERT_FAIL) {
            kill(getpid(), SIGABRT);
            while (1)
                ;
        }
        ASSERT(0 && "Plugin exit callback called with incorrect REASON.");
    }
}

void
vcpu_mem_capture(unsigned int cpu_index, qemu_plugin_meminfo_t info,
                 uint64_t vaddr, void *udata)
{
    uint64_t tid = get_task_id();
    __perf_transition_qemu(tid, STATE_Q_GUEST, STATE_Q_PLUGIN, false);

    context_t ctx       = *(context_t *)udata;
    CPUARMState *armcpu = qlotto_get_armcpu(cpu_index);
    _set_ctx_id(&ctx, cpu_index);

    gdb_handle_mem_capture(cpu_index, info, vaddr, udata);

    if (_should_drop(&ctx, true)) {
        __perf_transition_qemu(tid, STATE_Q_PLUGIN, STATE_Q_GUEST, false);
        return;
    }

    ctx.pstate = armcpu->pstate;

    // All memory categories have the address of the target memory location
    // and the pointer size as first two arguments
    ctx.args[0].width     = ARG_PTR;
    ctx.args[0].value.ptr = vaddr;

    ctx.args[1].width     = ARG_U64;
    ctx.args[1].value.u64 = sizeof(uint64_t);

    // Memory accesses that modify the memory also carry the new value.
    // TODO: replace dummy with actual value
    if (ctx.cat == CAT_BEFORE_AWRITE) {
        // arguments 1 for AWRITE
        ctx.args[2].width     = ARG_U64;
        ctx.args[2].value.u64 = 0;
    }

    if (ctx.cat == CAT_BEFORE_CMPXCHG) {
        // arguments 1 and 2 for CMPXCHG
        ctx.args[2].width = ARG_U64;
        // TODO: replace dummy with actual value
        ctx.args[2].value.u64 = 0;

        ctx.args[3].width = ARG_U64;
        // TODO: replace dummy with actual value
        ctx.args[3].value.u64 = 1;
    }

    _intercept(&ctx, __FUNCTION__);
    __perf_transition_qemu(tid, STATE_Q_PLUGIN, STATE_Q_GUEST, false);
}

void
vcpu_insn_capture(unsigned int cpu_index, void *udata)
{
    uint64_t tid = get_task_id();
    __perf_transition_qemu(tid, STATE_Q_GUEST, STATE_Q_PLUGIN, false);

    context_t ctx       = *(context_t *)udata;
    CPUARMState *armcpu = qlotto_get_armcpu(cpu_index);

    _set_ctx_id(&ctx, cpu_index);

    // TODO: it seems that ctx is not being really reset, and sometimes
    // ctx.func contains garbage.
    //       To avoid SEGFAULTs, we always set it here.
    ctx.func = __FUNCTION__;

    if (ctx.cat == CAT_RSRC_ACQUIRING || ctx.cat == CAT_RSRC_RELEASED) {
        ctx.args[0] = arg_ptr(armcpu->xregs[20]);
    }

    _intercept(&ctx, __FUNCTION__);
    __perf_transition_qemu(tid, STATE_Q_PLUGIN, STATE_Q_GUEST, false);
}

void
vcpu_loop_capture(unsigned int cpu_index, void *udata)
{
    uint64_t tid = get_task_id();
    __perf_transition_qemu(tid, STATE_Q_GUEST, STATE_Q_PLUGIN, false);

    context_t ctx = *(context_t *)udata;

    _set_ctx_id(&ctx, cpu_index);

    uint64_t reduction = 1000; // 100000
    frontend_state.inst_loop_count++;

    if (frontend_state.inst_loop_count % reduction == 0) {
        // TODO: it seems that ctx is not being really reset, and sometimes
        // ctx.func contains garbage.
        //       To avoid SEGFAULTs, we always set it here.
        ctx.func = __FUNCTION__;

        _intercept(&ctx, __FUNCTION__);
    }
    __perf_transition_qemu(tid, STATE_Q_PLUGIN, STATE_Q_GUEST, false);
}

void
vcpu_event_capture(unsigned int cpu_index, void *udata)
{
    uint64_t tid = get_task_id();
    __perf_transition_qemu(tid, STATE_Q_GUEST, STATE_Q_PLUGIN, false);

    event_context_t *evctx = (event_context_t *)udata;
    eventi_t *event        = evctx->event;
    context_t *ctx         = evctx->ctx;
    CPUARMState *armcpu    = qlotto_get_armcpu(cpu_index);

    _set_ctx_id(ctx, cpu_index);

    if (ctx->cat == CAT_RSRC_ACQUIRING) {
        // ctx.func_addr = pc;

        ctx->args[0] = arg_ptr(armcpu->xregs[0]);
        if (event->ti.key == 0xffffffe0028a179c)
            ctx->args[0] = arg_ptr(armcpu->xregs[0] + 0xbf4);
        if (event->ti.key == 0xffffffe0028dcfc0)
            ctx->args[0] = arg_ptr(0xffffffe002a15ec0);
    }

    if (ctx->cat == CAT_RSRC_RELEASED) {
        // ctx.func_addr = pc;
        ctx->args[0] = arg_ptr(armcpu->xregs[0]);
        if (event->ti.key == 0xffffffe0028a183c)
            ctx->args[0] = arg_ptr(armcpu->xregs[0] + 0xbf4);
        if (event->ti.key == 0xffffffe0028dc768)
            ctx->args[0] = arg_ptr(0xffffffe002a15ec0);
    }

    uint64_t old_pc = armcpu->pc;
    armcpu->pc      = event->ti.key;
    gdb_set_stop_signal(5);
    gdb_set_stop_reason(STOP_REASON_swbreak, 0);
    _intercept(ctx, event->func_name);
    armcpu->pc = old_pc;
    __perf_transition_qemu(tid, STATE_Q_PLUGIN, STATE_Q_GUEST, false);
}

void
vcpu_exit_capture(unsigned int cpu_index, void *udata)
{
    uint64_t tid = get_task_id();
    __perf_transition_qemu(tid, STATE_Q_GUEST, STATE_Q_PLUGIN, false);

    context_t ctx = *(context_t *)udata;

    _set_ctx_id(&ctx, cpu_index);
    _intercept(&ctx, NULL);
    __perf_transition_qemu(tid, STATE_Q_PLUGIN, STATE_Q_GUEST, false);
}

void
frontend_init(void)
{
    frontend_state.cpu_index_offset = -1;
    frontend_state.sample_counter   = prng_seed();
    icounter_init(&frontend_state.inline_insn_count);
    icounter_init(&frontend_state.inline_insn_count_wfe);
    icounter_init(&frontend_state.inline_insn_count_wfi);
    frontend_perf_init();
}

void
frontend_exit(void)
{
    frontend_perf_exit();
    icounter_free(&frontend_state.inline_insn_count_wfi);
    icounter_free(&frontend_state.inline_insn_count_wfe);
    icounter_free(&frontend_state.inline_insn_count);
}
