/*
 */

#include <stdbool.h>
#include <stdint.h>

#include <lotto/base/task_id.h>
#include <lotto/perf/perf.h>
#include <lotto/qemu/lotto_udf.h>
#include <lotto/qlotto/frontend/interceptor.h>
#include <lotto/qlotto/frontend/perf.h>
#include <lotto/qlotto/qemu/armcpu.h>
#include <lotto/qlotto/qemu/callbacks.h>
#include <lotto/runtime/intercept.h>

#define MAX_CPUS 1024

#if defined LOTTO_PERF_EXAMPLE
    #include <glib.h>

typedef enum hist_type {
    HIST_TRANS,
    HIST_WALLCLOCK,
    HIST_VCLOCK,
    HIST_NUM,
} hist_t;

WEAK uint64_t get_watchdog_trigger_count();
WEAK uint64_t get_lotto_entry_count();
WEAK uint64_t get_lotto_clk();
WEAK uint64_t get_preemption_count();
WEAK uint64_t get_task_sched_count();
#endif

REGISTER_GLOBAL_DEFINE(into_guest_from_qemu, void, uint32_t cpu_index)
REGISTER_GLOBAL_DEFINE(into_qemu_from_guest, void, uint32_t cpu_index)
REGISTER_GLOBAL_DEFINE(into_qemu_from_guest_unsafe, void, uint32_t cpu_index)

typedef struct frontend_perf_state_s {
    uint64_t wtime_startup;
    uint64_t wtime_shutdown;

    bool exec_guest[MAX_TASKS];
    uint64_t __perf_time_into_engine[MAX_TASKS];
    uint64_t __perf_time_into_runtime[MAX_TASKS];
    uint64_t __perf_time_engine[MAX_TASKS];
    uint64_t __perf_time_runtime[MAX_TASKS];

#if defined LOTTO_PERF_EXAMPLE
    uint64_t kick_count;
    uint64_t read_count[MAX_TASKS];
    uint64_t write_count[MAX_TASKS];

    GHashTable *uniq_trace;
    GHashTable *hist_trace;
    GHashTable *icount_trace;
    GHashTable *wclock_trace;
    GHashTable *wfe_trace;
    GHashTable *wfi_trace;
    GHashTable *watchdog_trace;
    GHashTable *kick_trace;
    GHashTable *lotto_entry_trace;
    GHashTable *write_trace[MAX_TASKS];
    GHashTable *read_trace[MAX_TASKS];
    GHashTable *sched_count[MAX_TASKS];
    GHashTable *uniq[HIST_NUM][MAX_TASKS];
    GHashTable *hist[HIST_NUM][MAX_TASKS];
    GHashTable *lotto_clk_trace;
    GHashTable *preemption_trace;

    nanosec_t time_qemu;
    nanosec_t time_guest;
    nanosec_t time_lotto;

    nanosec_t time_qemu_tid[MAX_TASKS];
    nanosec_t time_guest_tid[MAX_TASKS];
    nanosec_t time_lotto_tid[MAX_TASKS];

    int32_t hist_count[HIST_NUM][MAX_TASKS];

    char perf_file_name[1024];
    FILE *perf_file;
#endif
} frontend_perf_state_t;

static frontend_perf_state_t frontend_perf_state;

bool
in_guest(task_id tid)
{
    return frontend_perf_state.exec_guest[tid];
}

void
into_guest_from_qemu(uint32_t cpu_index)
{
    task_id tid = get_task_id();

    frontend_perf_state.exec_guest[tid] = true;

    __perf_transition_qemu(tid, STATE_Q_QEMU, STATE_Q_GUEST, false);
}

void
into_qemu_from_guest(uint32_t cpu_index)
{
    task_id tid = get_task_id();

    frontend_perf_state.exec_guest[tid] = false;

    __perf_transition_qemu(tid, STATE_Q_GUEST, STATE_Q_QEMU, false);
}

void
into_qemu_from_guest_unsafe(uint32_t cpu_index)
{
    task_id tid = get_task_id();

    __perf_transition_qemu(tid, STATE_Q_GUEST, STATE_Q_QEMU, true);
}

void
qemu_cpu_kick_cb(void *cpu)
{
#if defined LOTTO_PERF_EXAMPLE
    frontend_perf_state.kick_count++;
#endif
}

void
vcpu_wfe_capture(unsigned int cpu_index, void *udata)
{
    // uint64_t tid = get_task_id();
    // __perf_transition_qemu(tid, STATE_Q_GUEST, STATE_Q_PLUGIN, false);
    // context_t ctx       = *(context_t *)udata;
    // CPUARMState *armcpu = qlotto_get_armcpu(cpu_index);
    // log_debugf("[%lu] WFI executed!\n", get_task_id());
    // __perf_transition_qemu(tid, STATE_Q_PLUGIN, STATE_Q_GUEST, false);
}

void
vcpu_wfi_capture(unsigned int cpu_index, void *udata)
{
    // uint64_t tid = get_task_id();
    // __perf_transition_qemu(tid, STATE_Q_GUEST, STATE_Q_PLUGIN, false);
    // context_t ctx       = *(context_t *)udata;
    // CPUARMState *armcpu = qlotto_get_armcpu(cpu_index);
    // log_debugf("[%lu] WFI executed!\n", get_task_id());
    // __perf_transition_qemu(tid, STATE_Q_PLUGIN, STATE_Q_GUEST, false);
}

void
vcpu_trace_start_capture(unsigned int cpu_index, void *udata)
{
    uint64_t tid = get_task_id();
    __perf_transition_qemu(tid, STATE_Q_GUEST, STATE_Q_PLUGIN, false);

#if defined LOTTO_PERF_EXAMPLE
    // context_t ctx = *(context_t *)udata;
    CPUARMState *armcpu = qlotto_get_armcpu(cpu_index);
    uint64_t trace_id   = armcpu->xregs[UDF_REG_ARG_1];
    // into_qemu(cpu_index);

    // if( (trace_id & 0xFFFFF) == 0x0c008) {
    //     logger_set_level(LOG_DEBUG);
    // }

    // clean up uniq executed tbs
    g_hash_table_remove_all(frontend_perf_state.uniq_trace);

    // save current instruction counter
    g_hash_table_insert(
        frontend_perf_state.icount_trace, (gpointer)(uintptr_t)trace_id,
        (gpointer)(uintptr_t)icounter_get(get_instruction_counter()));

    // save current wall clock
    g_hash_table_insert(frontend_perf_state.wclock_trace,
                        (gpointer)(uintptr_t)trace_id,
                        (gpointer)(uintptr_t)now());

    // save current wfe counter
    g_hash_table_insert(
        frontend_perf_state.wfe_trace, (gpointer)(uintptr_t)trace_id,
        (gpointer)(uintptr_t)icounter_get(get_instruction_counter_wfe()));

    // save current wfi counter
    g_hash_table_insert(
        frontend_perf_state.wfi_trace, (gpointer)(uintptr_t)trace_id,
        (gpointer)(uintptr_t)icounter_get(get_instruction_counter_wfi()));

    // save watchdog trigger counter
    uint64_t watchdog_count = 0;
    if (NULL != get_watchdog_trigger_count)
        watchdog_count = get_watchdog_trigger_count();
    g_hash_table_insert(frontend_perf_state.watchdog_trace,
                        (gpointer)(uintptr_t)trace_id,
                        (gpointer)(uintptr_t)watchdog_count);

    // save current wfi counter
    g_hash_table_insert(frontend_perf_state.kick_trace,
                        (gpointer)(uintptr_t)trace_id,
                        (gpointer)(uintptr_t)frontend_perf_state.kick_count);

    // save lotto entry counter
    uint64_t lotto_entry_count = 0;
    if (NULL != get_lotto_entry_count)
        lotto_entry_count = get_lotto_entry_count();
    g_hash_table_insert(frontend_perf_state.lotto_entry_trace,
                        (gpointer)(uintptr_t)trace_id,
                        (gpointer)(uintptr_t)lotto_entry_count);

    for (uint64_t tid = 0; tid < MAX_TASKS; tid++) {
        // log_infof( "[%lu] Inserting read count %lu\n", tid,
        // frontend_perf_state.read_count[tid]);
        g_hash_table_insert(
            frontend_perf_state.read_trace[tid], (gpointer)(uintptr_t)trace_id,
            (gpointer)(uintptr_t)(frontend_perf_state.read_count[tid]));
        g_hash_table_insert(
            frontend_perf_state.write_trace[tid], (gpointer)(uintptr_t)trace_id,
            (gpointer)(uintptr_t)(frontend_perf_state.write_count[tid]));
    }

    uint64_t lotto_clk_count = 0;
    if (NULL != get_lotto_clk)
        lotto_clk_count = get_lotto_clk();
    g_hash_table_insert(frontend_perf_state.lotto_clk_trace,
                        (gpointer)(uintptr_t)trace_id,
                        (gpointer)(uintptr_t)lotto_clk_count);

    uint64_t preemption_count = 0;
    if (NULL != get_preemption_count)
        preemption_count = get_preemption_count();
    g_hash_table_insert(frontend_perf_state.preemption_trace,
                        (gpointer)(uintptr_t)trace_id,
                        (gpointer)(uintptr_t)preemption_count);

    for (uint64_t tid = 0; tid < MAX_TASKS; tid++) {
        uint64_t sched_count = 0;
        if (NULL != get_task_sched_count)
            sched_count = get_task_sched_count(tid);
        g_hash_table_insert(frontend_perf_state.sched_count[tid],
                            (gpointer)(uintptr_t)trace_id,
                            (gpointer)(uintptr_t)(sched_count));
    }

    if (0) {
        if (trace_id < UDF_TRACE_END_) {
            log_infof("Trace [%s] start\n", udf_trace_str(trace_id));
        } else {
            log_infof("Trace [0x%lx] start\n", trace_id & 0xFFFFF);
        }
    }
#endif

    __perf_transition_qemu(tid, STATE_Q_PLUGIN, STATE_Q_GUEST, false);
}

void
vcpu_trace_end_capture(unsigned int cpu_index, void *udata)
{
    uint64_t tid = get_task_id();
    __perf_transition_qemu(tid, STATE_Q_GUEST, STATE_Q_PLUGIN, false);


#if defined LOTTO_PERF_EXAMPLE
    // context_t ctx = *(context_t *)udata;
    CPUARMState *armcpu = qlotto_get_armcpu(cpu_index);
    uint64_t trace_id   = armcpu->xregs[UDF_REG_ARG_1];

    uint64_t uniq_num     = g_hash_table_size(frontend_perf_state.uniq_trace);
    uint64_t icount_count = icounter_get(get_instruction_counter());
    uint64_t icount_diff  = icount_count - (uintptr_t)g_hash_table_lookup(
                                               frontend_perf_state.icount_trace,
                                               (gpointer)trace_id);
    uint64_t wclock_count = now();
    uint64_t wclock_diff  = wclock_count - (uintptr_t)g_hash_table_lookup(
                                               frontend_perf_state.wclock_trace,
                                               (gpointer)trace_id);
    uint64_t wfe_count = icounter_get(get_instruction_counter_wfe());
    uint64_t wfe_diff =
        wfe_count - (uintptr_t)g_hash_table_lookup(
                        frontend_perf_state.wfe_trace, (gpointer)trace_id);
    uint64_t wfi_count = icounter_get(get_instruction_counter_wfi());
    uint64_t wfi_diff =
        wfi_count - (uintptr_t)g_hash_table_lookup(
                        frontend_perf_state.wfi_trace, (gpointer)trace_id);

    uint64_t watchdog_count = 0;
    if (NULL != get_watchdog_trigger_count)
        watchdog_count = get_watchdog_trigger_count();
    uint64_t watchdog_diff =
        watchdog_count -
        (uintptr_t)g_hash_table_lookup(frontend_perf_state.watchdog_trace,
                                       (gpointer)trace_id);
    uint64_t kick_count = frontend_perf_state.kick_count;
    uint64_t kick_diff =
        kick_count - (uintptr_t)g_hash_table_lookup(
                         frontend_perf_state.kick_trace, (gpointer)trace_id);

    uint64_t lotto_entry_count = 0;
    if (NULL != get_lotto_entry_count)
        lotto_entry_count = get_lotto_entry_count();
    uint64_t lotto_entry_diff =
        lotto_entry_count -
        (uintptr_t)g_hash_table_lookup(frontend_perf_state.lotto_entry_trace,
                                       (gpointer)trace_id);

    uint64_t read_count[MAX_TASKS];
    uint64_t read_diff[MAX_TASKS];
    uint64_t write_count[MAX_TASKS];
    uint64_t write_diff[MAX_TASKS];

    for (uint64_t tid = 0; tid < 16; tid++) {
        read_count[tid]  = frontend_perf_state.read_count[tid];
        write_count[tid] = frontend_perf_state.write_count[tid];

        read_diff[tid] =
            read_count[tid] -
            (uintptr_t)g_hash_table_lookup(frontend_perf_state.read_trace[tid],
                                           (gpointer)trace_id);
        write_diff[tid] =
            write_count[tid] -
            (uintptr_t)g_hash_table_lookup(frontend_perf_state.write_trace[tid],
                                           (gpointer)trace_id);
    }

    uint64_t lotto_clk_count = 0;
    if (NULL != get_lotto_clk)
        lotto_clk_count = get_lotto_clk();
    uint64_t lotto_clk_diff =
        lotto_clk_count -
        (uintptr_t)g_hash_table_lookup(frontend_perf_state.lotto_clk_trace,
                                       (gpointer)trace_id);

    uint64_t preemption_count = 0;
    if (NULL != get_preemption_count)
        preemption_count = get_preemption_count();
    uint64_t preemption_diff =
        preemption_count -
        (uintptr_t)g_hash_table_lookup(frontend_perf_state.preemption_trace,
                                       (gpointer)trace_id);

    uint64_t sched_count[MAX_TASKS];
    uint64_t sched_diff[MAX_TASKS];

    for (uint64_t tid = 0; tid < MAX_TASKS; tid++) {
        sched_count[tid] = 0;
        sched_diff[tid]  = 0;
        if (NULL != get_task_sched_count) {
            sched_count[tid] = get_task_sched_count(tid);
            sched_diff[tid] =
                sched_count[tid] -
                (uintptr_t)g_hash_table_lookup(
                    frontend_perf_state.sched_count[tid], (gpointer)trace_id);
        }
    }

    // log_infof( "Adding hist %u value %u : %lu\n", h,
    // frontend_perf_state.hist_count[h], uniq_num);

    g_hash_table_insert(frontend_perf_state.hist_trace,
                        (gpointer)(uintptr_t)trace_id,
                        (gpointer)(uintptr_t)uniq_num);
    g_hash_table_remove_all(frontend_perf_state.uniq_trace);

    // into_qemu(cpu_index);

    if (0) {
        if (trace_id < UDF_TRACE_END_) {
            log_infof("Trace [%s] end\n", udf_trace_str(trace_id));
        } else {
            log_infof("Trace [0x%lx] end\n", trace_id & 0xFFFFF);
        }
        log_infof("  Uniq TB executed:      %11lu\n", uniq_num);
        log_infof("  WFE executed:          %11lu (%lu)\n", wfe_diff,
                  wfe_count);
        log_infof("  WFI executed:          %11lu (%lu)\n", wfi_diff,
                  wfi_count);
        log_infof("  Instructions executed: %11lu\n", icount_diff);
        log_infof("  Wall clock progress:   %11lu (ns)\n", wclock_diff);
        log_infof("  Watchdog triggers:     %11lu (%lu)\n", watchdog_diff,
                  watchdog_count);
        log_infof("  vCPU Kick count:       %11lu (%lu)\n", kick_diff,
                  kick_count);
        log_infof("  lotto entry count:     %11lu (%lu)\n", lotto_entry_diff,
                  lotto_entry_count);
    }

    if (1) {
        if (trace_id < UDF_TRACE_END_) {
            sys_fprintf(frontend_perf_state.perf_file, "%s,",
                        udf_trace_str(trace_id));
        } else {
            sys_fprintf(frontend_perf_state.perf_file, "%lx,",
                        trace_id & 0xFFFFF);
        }
        sys_fprintf(frontend_perf_state.perf_file, "%lu,", uniq_num);

        sys_fprintf(frontend_perf_state.perf_file, "%lu,%lu,", wfe_diff,
                    wfe_count);
        sys_fprintf(frontend_perf_state.perf_file, "%lu,%lu,", wfi_diff,
                    wfi_count);
        sys_fprintf(frontend_perf_state.perf_file, "%lu,%lu,", icount_diff,
                    icount_count);
        sys_fprintf(frontend_perf_state.perf_file, "%lu,%lu,", wclock_diff,
                    wclock_count);
        sys_fprintf(frontend_perf_state.perf_file, "%lu,%lu,", watchdog_diff,
                    watchdog_count);
        sys_fprintf(frontend_perf_state.perf_file, "%lu,%lu,", kick_diff,
                    kick_count);
        for (uint64_t tid = 1; tid < MAX_TASKS; tid++) {
            sys_fprintf(frontend_perf_state.perf_file, "%lu,%lu,",
                        read_diff[tid], read_count[tid]);
            sys_fprintf(frontend_perf_state.perf_file, "%lu,%lu,",
                        write_diff[tid], write_count[tid]);
        }
        sys_fprintf(frontend_perf_state.perf_file, "%lu,%lu,", lotto_entry_diff,
                    lotto_entry_count);
        sys_fprintf(frontend_perf_state.perf_file, "%lu,%lu,", lotto_clk_diff,
                    lotto_clk_count);
        for (uint64_t tid = 1; tid < MAX_TASKS; tid++) {
            sys_fprintf(frontend_perf_state.perf_file, "%lu,%lu,",
                        sched_diff[tid], sched_count[tid]);
        }
        sys_fprintf(frontend_perf_state.perf_file, "%lu,%lu", preemption_diff,
                    preemption_count);
        sys_fprintf(frontend_perf_state.perf_file, "\n");
    }

    logger_set_level(LOG_ERROR);
#endif
    __perf_transition_qemu(tid, STATE_Q_PLUGIN, STATE_Q_GUEST, false);
}

void
frontend_perf_init(void)
{
    frontend_perf_state.wtime_startup = now();

    REGISTER_CALL(in_guest)

    REGISTER_CALL(into_qemu_from_guest)
    REGISTER_CALL(into_qemu_from_guest_unsafe)
    REGISTER_CALL(into_guest_from_qemu)

#if defined LOTTO_PERF_EXAMPLE
    for (uint64_t tid = 0; tid < MAX_TASKS; tid++) {
        for (int i = 0; i < HIST_NUM; i++) {
            frontend_perf_state.uniq[i][tid] =
                g_hash_table_new(g_direct_hash, g_direct_equal);
            frontend_perf_state.hist[i][tid] =
                g_hash_table_new(g_direct_hash, g_direct_equal);
            frontend_perf_state.hist_count[i][tid] = 0;
        }
    }
    frontend_perf_state.uniq_trace =
        g_hash_table_new(g_direct_hash, g_direct_equal);
    frontend_perf_state.hist_trace =
        g_hash_table_new(g_direct_hash, g_direct_equal);
    frontend_perf_state.icount_trace =
        g_hash_table_new(g_direct_hash, g_direct_equal);
    frontend_perf_state.wclock_trace =
        g_hash_table_new(g_direct_hash, g_direct_equal);
    frontend_perf_state.wfe_trace =
        g_hash_table_new(g_direct_hash, g_direct_equal);
    frontend_perf_state.wfi_trace =
        g_hash_table_new(g_direct_hash, g_direct_equal);
    frontend_perf_state.watchdog_trace =
        g_hash_table_new(g_direct_hash, g_direct_equal);
    frontend_perf_state.kick_trace =
        g_hash_table_new(g_direct_hash, g_direct_equal);
    frontend_perf_state.lotto_entry_trace =
        g_hash_table_new(g_direct_hash, g_direct_equal);

    for (uint64_t tid = 0; tid < MAX_TASKS; tid++) {
        frontend_perf_state.read_trace[tid] =
            g_hash_table_new(g_direct_hash, g_direct_equal);
        frontend_perf_state.write_trace[tid] =
            g_hash_table_new(g_direct_hash, g_direct_equal);
        frontend_perf_state.sched_count[tid] =
            g_hash_table_new(g_direct_hash, g_direct_equal);
    }
    frontend_perf_state.lotto_clk_trace =
        g_hash_table_new(g_direct_hash, g_direct_equal);
    frontend_perf_state.preemption_trace =
        g_hash_table_new(g_direct_hash, g_direct_equal);

    sys_snprintf(frontend_perf_state.perf_file_name, 1024, "perf_lotto.csv");
    frontend_perf_state.perf_file =
        sys_fopen(frontend_perf_state.perf_file_name, "w");

    sys_fprintf(frontend_perf_state.perf_file, "Tracepoint,");
    sys_fprintf(frontend_perf_state.perf_file, "Uniq TB,");
    sys_fprintf(frontend_perf_state.perf_file, "WFE,WFE total,");
    sys_fprintf(frontend_perf_state.perf_file, "WFI,WFI total,");
    sys_fprintf(frontend_perf_state.perf_file,
                "Instructions,Instructions total,");
    sys_fprintf(frontend_perf_state.perf_file, "Wall clock,Wall clock total,");
    sys_fprintf(frontend_perf_state.perf_file, "Watchdog,Watchdog total,");
    sys_fprintf(frontend_perf_state.perf_file, "vCPU Kicks,vCPU Kicks total,");
    for (uint64_t tid = 1; tid < MAX_TASKS; tid++) {
        sys_fprintf(frontend_perf_state.perf_file,
                    "reads %lu, reads total %lu,", tid, tid);
        sys_fprintf(frontend_perf_state.perf_file,
                    "writes %lu, writes total %lu,", tid, tid);
    }
    sys_fprintf(frontend_perf_state.perf_file,
                "lotto entry,lotto entry total,");
    sys_fprintf(frontend_perf_state.perf_file, "lotto clk,lotto clk total,");
    for (uint64_t tid = 1; tid < MAX_TASKS; tid++) {
        sys_fprintf(frontend_perf_state.perf_file,
                    "Task %lu scheduled,Task %lu scheduled total,", tid, tid);
    }
    sys_fprintf(frontend_perf_state.perf_file,
                "lotto preemptions,lotto preemptions total");

    sys_fprintf(frontend_perf_state.perf_file, "\n");
#endif
}

void
frontend_perf_exit(void)
{
    frontend_perf_state.wtime_shutdown = now();

#if defined LOTTO_PERF_EXAMPLE
    nanosec_t global_time_total = 0;
    nanosec_t global_time_qemu  = frontend_perf_state.time_qemu;
    nanosec_t global_time_lotto = frontend_perf_state.time_lotto;
    nanosec_t global_time_guest = frontend_perf_state.time_guest;

    global_time_total =
        global_time_qemu + global_time_lotto + global_time_guest;

    double global_percent_qemu =
        100.0 * (double)global_time_qemu / (double)global_time_total;
    double global_percent_lotto =
        100.0 * (double)global_time_lotto / (double)global_time_total;
    double global_percent_guest =
        100.0 * (double)global_time_guest / (double)global_time_total;

    uint64_t time_diff =
        frontend_perf_state.wtime_shutdown - frontend_perf_state.wtime_startup;

    uint64_t iic = icounter_get(get_instruction_counter());
    log_infof("\n");
    log_infof("Inst / sec: %.3f (total time)\n",
              (double)iic / ((double)time_diff / (double)NOW_SECOND));
    log_infof("Inst / sec: %.3f (guest time)\n",
              (double)iic / ((double)global_time_guest / (double)NOW_SECOND));
    log_infof("### global time usage: ###\n");
    log_infof("   Time spent in  QEmu: %6.3f%%\n", global_percent_qemu);
    log_infof("   Time spent in Lotto: %6.3f%%\n", global_percent_lotto);
    log_infof("   Time spent in guest: %6.3f%%\n", global_percent_guest);
    log_infof("\n");
    log_infof("### per-thread [task id] time usage: ###\n");
    for (uint64_t i = 1; i < get_task_count(); i++) {
        nanosec_t local_time_total = 0;
        nanosec_t local_time_qemu  = frontend_perf_state.time_qemu_tid[i];
        nanosec_t local_time_guest = frontend_perf_state.time_guest_tid[i];
        nanosec_t local_time_lotto = frontend_perf_state.time_lotto_tid[i];

        local_time_total =
            local_time_qemu + local_time_guest + local_time_lotto;

        double local_percent_qemu =
            100.0 * (double)local_time_qemu / (double)local_time_total;
        double local_percent_guest =
            100.0 * (double)local_time_guest / (double)local_time_total;
        double local_percent_lotto =
            100.0 * (double)local_time_lotto / (double)local_time_total;

        log_infof("   [%lu] Time spent in     QEmu: %6.3f%%\n", i,
                  local_percent_qemu);
        log_infof("   [%lu] Time spent in    Lotto: %6.3f%%\n", i,
                  local_percent_lotto);
        log_infof("   [%lu] Time spent in    guest: %6.3f%%\n", i,
                  local_percent_guest);
        log_infof("\n");
    }
    __perf_print_results();

    // log_infof( "Histogram of uniq tb per transition
    // guest->lotto\n");
    // g_hash_table_foreach(frontend_perf_state.hist[HIST_TRANS], print_hash,
    // NULL);

    for (uint64_t tid = 1; tid < get_task_count(); tid++) {
        uint64_t num_entries = frontend_perf_state.hist_count[HIST_TRANS][tid];
        log_infof("[%lu] %lu entries\n", tid, num_entries);
        char fn[1024];
        sys_snprintf(fn, 1024, "uniq_tb_hist_%lu.log", tid);
        FILE *f = sys_fopen(fn, "w");
        for (uint32_t i = 0; i < num_entries; i++) {
            uint64_t num = i;
            uint64_t val = (uintptr_t)g_hash_table_lookup(
                frontend_perf_state.hist[HIST_TRANS][tid],
                (gpointer)(uintptr_t)num);
            sys_fprintf(f, "%lu,", val);
            // log_infof( "[%lu] %7lu: %7lu\n", tid, num, val);
        }
        log_infof("\n");
        sys_fclose(f);
    }

    sys_fclose(frontend_perf_state.perf_file);
#endif
}
