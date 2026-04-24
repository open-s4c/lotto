/**
 * @file state.h
 * @brief Termination module state declarations.
 */
#ifndef LOTTO_STATE_TERMINATION_H
#define LOTTO_STATE_TERMINATION_H

#include <stdbool.h>
#include <stdint.h>

#include <lotto/base/marshable.h>
#include <lotto/util/macros.h>

#define FOR_EACH_TERMINATE_MODE                                              \
    GEN_TERMINATE_MODE(CLK)                                                  \
    GEN_TERMINATE_MODE(CHPT)                                                 \
    GEN_TERMINATE_MODE(PREEMPT)                                              \
    GEN_TERMINATE_MODE(REPLAY)                                               \
    GEN_TERMINATE_MODE(TIME)

#define GEN_TERMINATE_MODE(mode) TERMINATE_MODE_##mode,
typedef enum termination_mode {
    TERMINATE_MODE_NONE = 0,
    FOR_EACH_TERMINATE_MODE TERMINATE_MODE_END_,
    TERMINATE_MODE_SIZE_ = (1ULL << 62),
} terminate_mode_t;
#undef GEN_TERMINATE_MODE

#define TERMINATE_MODE_MAX_LEN     256
#define TERMINATE_MODE_MAX_STR_LEN 2048


typedef struct terminate_config {
    marshable_t m;
    terminate_mode_t mode;
    uint64_t limit;
} terminate_config_t;

/**
 * Returns a string of the given termination mode.
 *
 * @param mode termination mode
 * @return const char array
 */
const char *terminate_mode_str(terminate_mode_t mode);

/**
 * Returns the termination mode from `src`.
 *
 * @param src string of the form `VAL`.
 * @return termination mode
 */
terminate_mode_t terminate_mode_from(const char *src);

/**
 * Writes to a preallocated string all possible termination modes.
 *
 * @param s preallocated char array with at least
 * TERMINATE_MODE_MAX_STR_LEN bytes.
 */
void terminate_mode_all_str(char *str);

/**
 * Returns the termination handler config.
 */
terminate_config_t *terminate_config();

#endif
