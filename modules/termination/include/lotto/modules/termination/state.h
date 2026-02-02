#ifndef LOTTO_STATE_TERMINATION_H
#define LOTTO_STATE_TERMINATION_H

#include <stdbool.h>
#include <stdint.h>

#include <lotto/base/marshable.h>
#include <lotto/util/macros.h>

#define FOR_EACH_TERMINATION_MODE                                              \
    GEN_TERMINATION_MODE(CLK)                                                  \
    GEN_TERMINATION_MODE(CHPT)                                                 \
    GEN_TERMINATION_MODE(PREEMPT)                                              \
    GEN_TERMINATION_MODE(REPLAY)                                               \
    GEN_TERMINATION_MODE(TIME)

#define GEN_TERMINATION_MODE(mode) TERMINATION_MODE_##mode,
typedef enum termination_mode {
    TERMINATION_MODE_NONE = 0,
    FOR_EACH_TERMINATION_MODE TERMINATION_MODE_END_,
    TERMINATION_MODE_SIZE_ = (1ULL << 62),
} termination_mode_t;
#undef GEN_TERMINATION_MODE

#define TERMINATION_MODE_MAX_LEN     256
#define TERMINATION_MODE_MAX_STR_LEN 2048


typedef struct termination_config {
    marshable_t m;
    termination_mode_t mode;
    uint64_t limit;
} termination_config_t;

/**
 * Returns a string of the given termination mode.
 *
 * @param mode termination mode
 * @return const char array
 */
const char *termination_mode_str(termination_mode_t mode);

/**
 * Returns the termination mode from `src`.
 *
 * @param src string of the form `VAL`.
 * @return termination mode
 */
termination_mode_t termination_mode_from(const char *src);

/**
 * Writes to a preallocated string all possible termination modes.
 *
 * @param s preallocated char array with at least
 * TERMINATION_MODE_MAX_STR_LEN bytes.
 */
void termination_mode_all_str(char *str);

/**
 * Returns the termination handler config.
 */
termination_config_t *termination_config();

#endif
