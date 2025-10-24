/*
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <dlfcn.h>

// system
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/time.h>

// capstone
#include <capstone/arm64.h>
#include <capstone/capstone.h>

// qemu
#include <qemu-plugin.h>

// lotto
#include <lotto/qlotto/qemu/stubs.h>
#include <lotto/unsafe/_sys.h>
#include <lotto/unsafe/disable.h>
#include <lotto/unsafe/rogue.h>

#define LOG_FILE     "qemu_inst_measure.log"
#define MAX_MEASURES 10000000ULL

#define USEC_IN_SEC      1000000ULL
#define NSEC_IN_SEC      (USEC_IN_SEC * 1000ULL)
#define MEASURE_INTERVAL (NSEC_IN_SEC / 10ULL) // 100 milliseconds
#define REPORT_INTERVAL  (NSEC_IN_SEC * 2ULL)  // 2 seconds

#define WINDOW_SIZE (REPORT_INTERVAL / MEASURE_INTERVAL)

#define CLOCKID CLOCK_REALTIME

typedef struct measure_state_s {
    uint64_t datapoints[MAX_MEASURES];
    uint64_t timestamps[MAX_MEASURES];
    uint64_t num_samples;
    FILE *log_file;
    struct timeval tv_start;
    icounter_t inst_count;
    timer_t timerid_measure;
    timer_t timerid_report;
} measure_state_t;

static measure_state_t _state;

static bool measure_started = false;

/*******************************************************************************
 * measurement plugin for qemu performance
 ******************************************************************************/

QEMU_PLUGIN_EXPORT int qemu_plugin_version = QEMU_PLUGIN_VERSION;

icounter_t *
get_instruction_counter(void)
{
    return &_state.inst_count;
}

// in micro seconds since start of plugin
uint64_t
get_timestamp()
{
    uint64_t timestamp = 0;
    struct timeval tv, tv1;
    rl_gettimeofday(&tv, NULL);

    // current diff in seconds from start
    tv1.tv_sec = tv.tv_sec - _state.tv_start.tv_sec;

    // current diff in micro seconds from start
    tv1.tv_usec = tv.tv_usec - _state.tv_start.tv_usec;

    // if usec is <0 subtract one from second and add 1 second to usec
    if (tv1.tv_usec < 0) {
        tv1.tv_sec--;
        tv1.tv_usec += USEC_IN_SEC;
    }
    timestamp = ((uint64_t)tv1.tv_sec * 1000000 + tv1.tv_usec);
    return timestamp;
}

static void
handler_measure(void)
{
    _state.timestamps[_state.num_samples] = get_timestamp();
    _state.datapoints[_state.num_samples] = icounter_get(&_state.inst_count);
    _state.num_samples++;
}

static void
handler_report(void)
{
    if (!_state.log_file)
        return;
    if (_state.num_samples == 0)
        return;

    uint64_t cur_sample_idx = _state.num_samples - 1;
    uint64_t last_datapoint = _state.datapoints[cur_sample_idx];
    uint64_t last_timestamp = _state.timestamps[cur_sample_idx];

    uint64_t first_window_idx =
        (cur_sample_idx >= WINDOW_SIZE) ? (cur_sample_idx - WINDOW_SIZE) : 0;

    uint64_t first_datapoint = _state.datapoints[first_window_idx];
    uint64_t first_timestamp = _state.timestamps[first_window_idx];

    uint64_t inst_diff = last_datapoint - first_datapoint;
    uint64_t time_diff = last_timestamp - first_timestamp; // in microseconds

    if (inst_diff == 0 || time_diff == 0)
        return;

    uint64_t inst_per_second = inst_diff * USEC_IN_SEC / time_diff;

    rl_fprintf(_state.log_file, "[%10lu ms] %10lu inst/s\n",
               (last_timestamp / 1000), inst_per_second);
}

void *
measure_thread(void *data)
{
    struct timespec duration;
    duration.tv_sec      = 0;
    duration.tv_nsec     = (NSEC_IN_SEC / 10);
    uint64_t report_freq = WINDOW_SIZE;

    uint64_t counter = 0;
    lotto_rogue({
        while (1) {
            rl_nanosleep(&duration, NULL);
            counter++;
            handler_measure();
            if ((counter % report_freq) == 0)
                handler_report();
        }
    });
    return NULL;
}

static void
measure_start(void)
{
    pthread_t measure_t;

    if (pthread_create(&measure_t, 0, measure_thread, NULL) != 0) {
        printf("pthread_create() error on gdb_server_thread\n");
        exit(1);
    }
}

static void
plugin_exit(qemu_plugin_id_t id, void *p)
{
    printf("Plugin exit.\n");

    // Add computation & output of
    // mean, stddev, median, 0.1/1/10% lows and highs

    uint64_t num_samples = _state.num_samples;
    uint64_t last_idx    = num_samples - 1;
    uint64_t runtime     = _state.timestamps[last_idx];
    uint64_t inst_count  = _state.datapoints[last_idx];
    uint64_t mean        = inst_count * USEC_IN_SEC / runtime;

    fprintf(_state.log_file, "######### SUMMARY #########\n");
    fprintf(_state.log_file, "# Runtime:           %15lu usec\n", runtime);
    fprintf(_state.log_file, "# Instruction count: %15lu\n", inst_count);
    fprintf(_state.log_file, "# Average Instr./s:  %15lu\n", mean);
    fprintf(_state.log_file, "#########   END   #########\n");

    if (_state.log_file)
        fclose(_state.log_file);

    icounter_free(&_state.inst_count);
}

void
vcpu_tb_trans(qemu_plugin_id_t id, struct qemu_plugin_tb *tb)
{
    size_t n = qemu_plugin_tb_n_insns(tb);

    if (!measure_started) {
        measure_started = true;
        measure_start();
    }

    for (size_t i = 0; i < n; i++) {
        struct qemu_plugin_insn *insn = qemu_plugin_tb_get_insn(tb, i);

        /*
            Add inline counter of executed instructions
            Used for deterministic time progression in lotto
        */

        register_vcpu_insn_exec_inline(insn, QEMU_PLUGIN_INLINE_ADD_U64,
                                       get_instruction_counter(), 1);
    }
}

QEMU_PLUGIN_EXPORT int
qemu_plugin_install(qemu_plugin_id_t id, const qemu_info_t *info, int argc,
                    char **argv)
{
    qemu_plugin_register_vcpu_tb_trans_cb(id, vcpu_tb_trans);
    qemu_plugin_register_atexit_cb(id, plugin_exit, NULL);
    icounter_init(&_state.inst_count);

    _state.log_file    = NULL;
    _state.num_samples = 0;

    rl_gettimeofday(&_state.tv_start, NULL);
    _state.log_file = fopen(LOG_FILE, "a");
    if (!_state.log_file) {
        rl_fprintf(stderr, "Could not open log file for writing! (%s)",
                   LOG_FILE);
        exit(1);
    }

    {
        time_t t      = rl_time(NULL);
        struct tm *tm = localtime(&t);
        char s[64];
        size_t ret = strftime(s, sizeof(s), "%c", tm);
        assert(ret);
        rl_fprintf(_state.log_file, "[%s] Starting QEmu time measurements\n",
                   s);
        fflush(_state.log_file);
    }

    rl_printf("Starting measurements on qemu.\n");

    return 0;
}
