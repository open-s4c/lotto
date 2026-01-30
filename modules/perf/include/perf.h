/*
 */
#ifndef LOTTO_RUNTIME_PERF_H
#define LOTTO_RUNTIME_PERF_H

#include <lotto/base/context.h>
#include <lotto/base/task_id.h>

#define MAX_TASKS (1024 + 5)

#define FOR_EACH_STATE_L                                                       \
    GEN_STATE(RUNTIME)                                                         \
    GEN_STATE(ENGINE)                                                          \
    GEN_STATE(CALL)                                                            \
    GEN_STATE(SWITCHER)                                                        \
    GEN_STATE(DETACHED)

#define GEN_STATE(state) STATE_L_##state,
typedef enum perf_lotto_state {
    STATE_L_NONE = 0,
    FOR_EACH_STATE_L STATE_L_END_
} perf_lotto_state_t;
#undef GEN_STATE

/**
 * Returns a string representation of the state_l name.
 */
const char *perf_lotto_state_str(perf_lotto_state_t t);
static inline const char *
state_l_str(perf_lotto_state_t t)
{
    return perf_lotto_state_str(t);
}

#define FOR_EACH_STATE_Q                                                       \
    GEN_STATE(QEMU)                                                            \
    GEN_STATE(PLUGIN)                                                          \
    GEN_STATE(GUEST)

#define GEN_STATE(state) STATE_Q_##state,
typedef enum perf_qemu_state {
    STATE_Q_NONE = 0,
    FOR_EACH_STATE_Q STATE_Q_END_
} perf_qemu_state_t;
#undef GEN_STATE

/**
 * Returns a string representation of the state_l name.
 */
const char *perf_qemu_state_str(perf_qemu_state_t t);
static inline const char *
state_q_str(perf_qemu_state_t t)
{
    return perf_qemu_state_str(t);
}

typedef struct {
    perf_qemu_state_t state_qemu;
    perf_lotto_state_t state_lotto;
    uint64_t num_transistions;
} perf_sm_t;

extern perf_sm_t __state_qlotto[MAX_TASKS];

void __perf_measure_qemu(task_id tid, perf_qemu_state_t old,
                         perf_qemu_state_t new);
void __perf_measure_lotto(task_id tid, perf_lotto_state_t old,
                          perf_lotto_state_t new);
void __perf_print_results(void);

static inline void
__perf_debug_call(void)
{
    return;
}

static inline void
__perf_transition_lotto(task_id tid, perf_lotto_state_t old_state,
                        perf_lotto_state_t new_state)
{
#if defined(LOTTO_PERF) && !defined(LOTTO_TEST)
    ASSERT(tid < MAX_TASKS);

    // state_qemu can be anything, we can come from the plugin, we can come from
    // the plotto interceptor, we can come from a guest helper function taking a
    // mutex
    ASSERT(__state_qlotto[tid].state_qemu == STATE_Q_QEMU ||
           __state_qlotto[tid].state_qemu == STATE_Q_PLUGIN ||
           __state_qlotto[tid].state_qemu == STATE_Q_GUEST);

    // assert old state
    ASSERT(__state_qlotto[tid].state_lotto == old_state);

    // assert correct transition
    switch (new_state) {
        case STATE_L_ENGINE:
            // ENGINE must come from RUNTIME or SWITCHER
            ASSERT(__state_qlotto[tid].state_lotto == STATE_L_RUNTIME ||
                   __state_qlotto[tid].state_lotto == STATE_L_SWITCHER);
            break;
        case STATE_L_RUNTIME:
            // RUNTIME must come from ENGINE or DETACHED
            ASSERT(__state_qlotto[tid].state_lotto == STATE_L_ENGINE ||
                   __state_qlotto[tid].state_lotto == STATE_L_DETACHED);
            break;
        case STATE_L_CALL:
            // CALL must come from ENGINE
            ASSERT(__state_qlotto[tid].state_lotto == STATE_L_ENGINE);
            break;
        case STATE_L_SWITCHER:
            // SWITCHER must come from ENGINE or DETACHED
            ASSERT(__state_qlotto[tid].state_lotto == STATE_L_ENGINE ||
                   __state_qlotto[tid].state_lotto == STATE_L_DETACHED);
            break;
        case STATE_L_DETACHED:
            // DETACHED must come from CALL
            ASSERT(__state_qlotto[tid].state_lotto == STATE_L_CALL);
            break;
        default:
            // all target states need to be implemented
            ASSERT(0 && "Unknown new (target) state to check transition");
            break;
    }

    __state_qlotto[tid].state_lotto = new_state;

    // measure
    __perf_measure_lotto(tid, old_state, new_state);
    return;
#endif
}

static inline void
__perf_transition_qemu(task_id tid, perf_qemu_state_t old_state,
                       perf_qemu_state_t new_state, bool unsafe)
{
#if defined(LOTTO_PERF) && !defined(LOTTO_TEST)
    ASSERT(tid < MAX_TASKS);

    // if run for the first time, set states appropriately
    if (STATE_L_NONE == __state_qlotto[tid].state_lotto) {
        __state_qlotto[tid].state_lotto = STATE_L_RUNTIME;
    }

    if (STATE_Q_NONE == __state_qlotto[tid].state_qemu) {
        __state_qlotto[tid].state_qemu = STATE_Q_QEMU;
    }

    // state_lotto must be RUNTIME or NONE (if not initialized yet)
    ASSERT(__state_qlotto[tid].state_lotto == STATE_L_RUNTIME);

    // some qemu transition into STATE QEMU are unsafe as in they are occur
    // either from inside QEMU or GUEST, the same code paths are used in qemu
    // for these
    if (unsafe) {
        // unsafe transition only happens when going into QEMU
        ASSERT(STATE_Q_QEMU == new_state);
    } else {
        // assert old state on safe transitions
        ASSERT(__state_qlotto[tid].state_qemu == old_state);
    }

    switch (new_state) {
        case STATE_Q_PLUGIN:
            // PLUGIN must come from GUEST or QEMU
            ASSERT(__state_qlotto[tid].state_qemu == STATE_Q_GUEST ||
                   __state_qlotto[tid].state_qemu == STATE_Q_QEMU);
            break;
        case STATE_Q_QEMU:
            if (unsafe) {
                // QEMU must come from GUEST or QEMU (in unsafe transition)
                ASSERT(__state_qlotto[tid].state_qemu == STATE_Q_GUEST ||
                       __state_qlotto[tid].state_qemu == STATE_Q_QEMU);
            } else {
                // QEMU must come from GUEST or PLUGIN (in safe transition)
                ASSERT(__state_qlotto[tid].state_qemu == STATE_Q_GUEST ||
                       __state_qlotto[tid].state_qemu == STATE_Q_PLUGIN);
            }
            break;
        case STATE_Q_GUEST:
            // GUEST must come from PLUGIN or QEMU
            ASSERT(__state_qlotto[tid].state_qemu == STATE_Q_PLUGIN ||
                   __state_qlotto[tid].state_qemu == STATE_Q_QEMU);
            break;
        default:
            // all target states need to be implemented
            ASSERT(0 && "Unknown new (target) state to check transition");
            break;
    }

    // if we are in an unsafe transition, which does not change state, skip
    // measurement
    if (unsafe && __state_qlotto[tid].state_qemu == new_state) {
        return;
    }

    // measure (we dont use old_state here, is it might be wrong in unsafe
    // transition)
    __perf_measure_qemu(tid, __state_qlotto[tid].state_qemu, new_state);

    // set new state
    __state_qlotto[tid].state_qemu = new_state;

    // count number of transitions
    __state_qlotto[tid].num_transistions++;
    return;
#endif // LOTTO_PERF
}

#endif // LOTTO_RUNTIME_PERF_H
