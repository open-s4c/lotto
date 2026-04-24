
// system
#include <assert.h>
#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// capstone
#include <capstone/arm64.h>
#include <capstone/capstone.h>

// qemu
#include <qemu-plugin.h>

#include <lotto/modules/qemu/events.h>
#define ASSERT_DISABLE

#include <lotto/modules/qemu/stubs.h>
#include <lotto/qemu/lotto_udf.h>
#include <lotto/util/casts.h>

#define MAX_SNAPSHOTS 10
#define CLOCKID       CLOCK_REALTIME
#define SIG           SIGRTMIN

#define MAX_SNAPSHOT_STR_LEN 128

#define SNAPSHOT_REQUENCY 10ULL * 1000000000ULL // 10 seconds
#define SNAPSHOT_PREFIX   "ls_"

typedef struct snapshot_state_s {
    uint32_t snapshot_nr;
    char snapshot_str[MAX_SNAPSHOT_STR_LEN];
    csh disasm;
} snapshot_state_t;

snapshot_state_t snapshot_state;

void qemu_plugin_delayed_save_snapshot(const char *name);

void
qemu_snapshot(void)
{
    snapshot_state.snapshot_nr++;
    if (snapshot_state.snapshot_nr > MAX_SNAPSHOTS)
        snapshot_state.snapshot_nr = 1;

    snprintf(snapshot_state.snapshot_str, MAX_SNAPSHOT_STR_LEN,
             SNAPSHOT_PREFIX "%u", snapshot_state.snapshot_nr);

    // call function to capture qemu snapshot
    qemu_plugin_delayed_save_snapshot(snapshot_state.snapshot_str);

    printf("Created snapshot %s\n", snapshot_state.snapshot_str);
}

static void
vcpu_snapshot_capture(unsigned int cpu_index, void *udata)
{
    qemu_snapshot();
}

void
vcpu_exit_capture(unsigned int cpu_index, void *udata)
{
    kill(getpid(), SIGABRT);
    ASSERT(0 && "Unknown exit reason.");
}

inline static void
register_snapshot_cb(void *ctx, struct qemu_plugin_insn *insn)
{
    qemu_plugin_register_vcpu_insn_exec_cb(insn, vcpu_snapshot_capture,
                                           QEMU_PLUGIN_CB_R_REGS, ctx);
}

inline static void
register_exit_cb(void *ctx, struct qemu_plugin_insn *insn)
{
    qemu_plugin_register_vcpu_insn_exec_cb(insn, vcpu_exit_capture,
                                           QEMU_PLUGIN_CB_R_REGS, ctx);
}

static void
do_disasm_reg(struct qemu_plugin_insn *insn)
{
    uint32_t opcode = get_instruction_data(insn);

    switch (opcode) {
        case LOTTO_EXIT_SNAPSHOT_A64_VAL:
            register_snapshot_cb(NULL, insn);
            break;
        case LOTTO_EXIT_SUCCESS_VAL:
            register_exit_cb(NULL, insn);
            break;
        case LOTTO_EXIT_FAIL_VAL:
            register_exit_cb(NULL, insn);
            break;
        default:
            break;
    }
}

static void
vcpu_tb_trans(qemu_plugin_id_t id, struct qemu_plugin_tb *tb)
{
    size_t n = qemu_plugin_tb_n_insns(tb);

    for (size_t i = 0; i < n; i++) {
        struct qemu_plugin_insn *insn = qemu_plugin_tb_get_insn(tb, i);
        do_disasm_reg(insn);
    }
}

static void
handler_snapshot(int sig, siginfo_t *si, void *uc)
{
    qemu_snapshot();
}

static void
plugin_exit(qemu_plugin_id_t id, void *p)
{
    (void)id;
    (void)p;
    printf("Last snapshot created: ls_%u\n", snapshot_state.snapshot_nr);
}

void
qemu_snapshot_tb_translate(const qemu_translate_event_t *ev)
{
    if (ev == NULL) {
        return;
    }
    vcpu_tb_trans((qemu_plugin_id_t)ev->plugin_id, ev->tb);
}

void
qemu_snapshot_plugin_init(const qemu_control_event_t *ev)
{
    (void)ev;

    assert(cs_open(CS_ARCH_ARM64, CS_MODE_ARM, &snapshot_state.disasm) ==
           CS_ERR_OK);
    assert(cs_option(snapshot_state.disasm, CS_OPT_DETAIL, CS_OPT_ON) ==
           CS_ERR_OK);

    // Timer for snapshots
    timer_t timerid;
    uint64_t freq_nanosecs;
    struct sigevent sev;
    struct sigaction sa;
    struct itimerspec its;

    sa.sa_flags     = SA_SIGINFO;
    sa.sa_sigaction = handler_snapshot;
    sigemptyset(&sa.sa_mask);
    sigaction(SIG, &sa, NULL);

    sev.sigev_notify          = SIGEV_SIGNAL;
    sev.sigev_signo           = SIG;
    sev.sigev_value.sival_ptr = &timerid;
    timer_create(CLOCKID, &sev, &timerid);

    freq_nanosecs           = SNAPSHOT_REQUENCY;
    its.it_value.tv_sec     = CAST_TYPE(time_t, freq_nanosecs / 1000000000);
    its.it_value.tv_nsec    = CAST_TYPE(time_t, freq_nanosecs % 1000000000);
    its.it_interval.tv_sec  = its.it_value.tv_sec;
    its.it_interval.tv_nsec = its.it_value.tv_nsec;

    timer_settime(timerid, 0, &its, NULL);
}

void
qemu_snapshot_plugin_fini(const qemu_control_event_t *ev)
{
    plugin_exit((qemu_plugin_id_t)ev->plugin_id, NULL);
}
