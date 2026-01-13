/*
 */

#include <stdbool.h>

#include <lotto/base/context.h>
#include <lotto/base/task_id.h>
#include <lotto/perf/perf.h>
#include <lotto/qlotto/qemu/callbacks.h>
#include <lotto/sys/logger.h>
#include <lotto/sys/now.h>
#include <lotto/util/macros.h>

typedef struct {
    uint64_t __perf_time_lotto_into[MAX_TASKS][STATE_L_END_];
    uint64_t __perf_time_qemu_into[MAX_TASKS][STATE_Q_END_];
    uint64_t __perf_time_lotto[MAX_TASKS][STATE_L_END_];
    uint64_t __perf_time_qemu[MAX_TASKS][STATE_Q_END_];
} perf_measures_t;

perf_sm_t __state_qlotto[MAX_TASKS];
perf_measures_t perf_state;

#define GEN_STATE(state) [STATE_L_##state] = #state,
static const char *_perf_lotto_state_map[] = {[STATE_L_NONE] = "NONE",
                                              FOR_EACH_STATE_L};
#undef GEN_STATE

BIT_STR(perf_lotto_state, STATE_L)

#define GEN_STATE(state) [STATE_Q_##state] = #state,
static const char *_perf_qemu_state_map[] = {[STATE_Q_NONE] = "NONE",
                                             FOR_EACH_STATE_Q};
#undef GEN_STATE

BIT_STR(perf_qemu_state, STATE_Q)


uint64_t lotto_entry_count = 0;

uint64_t
get_lotto_entry_count(void)
{
    return lotto_entry_count;
}

REGISTER_GLOBAL_DEFINE(in_guest, bool, task_id tid)
REGISTER_GLOBAL_DEFINE(into_lotto_from_qemu, void, task_id tid)
REGISTER_GLOBAL_DEFINE(into_lotto_from_guest, void, task_id tid)
REGISTER_GLOBAL_DEFINE(into_qemu_from_lotto, void, task_id tid)
REGISTER_GLOBAL_DEFINE(into_guest_from_lotto, void, task_id tid)

// measuring for transistions between RUNTIME/ENGINE/SWITCHER/CALL/DETACHED
void
__perf_measure_lotto(task_id tid, perf_lotto_state_t old,
                     perf_lotto_state_t new)
{
    uint64_t cur_time = now();

    perf_state.__perf_time_lotto_into[tid][new] = cur_time;

    if (perf_state.__perf_time_lotto_into[tid][old] != 0) {
        perf_state.__perf_time_lotto[tid][old] +=
            cur_time - perf_state.__perf_time_lotto_into[tid][old];
    }
    return;
}

// measuring for transistions between QEMU/PLUGIN/GUEST
void
__perf_measure_qemu(task_id tid, perf_qemu_state_t old, perf_qemu_state_t new)
{
    uint64_t cur_time = now();

    perf_state.__perf_time_qemu_into[tid][new] = cur_time;

    if (perf_state.__perf_time_qemu_into[tid][old] != 0) {
        perf_state.__perf_time_qemu[tid][old] +=
            cur_time - perf_state.__perf_time_qemu_into[tid][old];
    }
    return;
}

void
__perf_print_results(void)
{
    for (uint64_t i = 1; i <= get_task_count(); i++) {
        uint64_t total_tid_lotto_time = 0;
        for (int sl = 0; sl < STATE_L_END_; sl++) {
            total_tid_lotto_time += perf_state.__perf_time_lotto[i][sl];
        }
        for (int sl = 1; sl < STATE_L_END_; sl++) {
            double perf_percent =
                (double)(perf_state.__perf_time_lotto[i][sl]) /
                (double)total_tid_lotto_time * 100.0;

            logger_infof("   [%lu] Time spent in %12s: %14lu\n", i,
                      state_l_str(sl), perf_state.__perf_time_lotto[i][sl]);
            logger_infof("   [%lu] Time spent in %12s: %6.3f%%\n", i,
                      state_l_str(sl), perf_percent);
        }
        logger_infof("\n");
    }

    for (uint64_t i = 1; i <= get_task_count(); i++) {
        uint64_t total_tid_lotto_time = 0;
        for (int sq = 0; sq < STATE_Q_END_; sq++) {
            total_tid_lotto_time += perf_state.__perf_time_qemu[i][sq];
        }
        for (int sq = 1; sq < STATE_Q_END_; sq++) {
            double perf_percent = (double)(perf_state.__perf_time_qemu[i][sq]) /
                                  (double)total_tid_lotto_time * 100.0;

            logger_infof("   [%lu] Time spent in %12s: %14lu\n", i,
                      state_q_str(sq), perf_state.__perf_time_qemu[i][sq]);
            logger_infof("   [%lu] Time spent in %12s: %6.3f%%\n", i,
                      state_q_str(sq), perf_percent);
        }
        logger_infof("\n");
    }
}
