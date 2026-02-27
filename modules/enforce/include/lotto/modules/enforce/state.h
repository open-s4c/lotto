#ifndef LOTTO_STATE_ENFORCE_H
#define LOTTO_STATE_ENFORCE_H

#include <stdbool.h>
#include <stdint.h>

#include <lotto/base/clk.h>
#include <lotto/base/context.h>
#include <lotto/base/stable_address.h>
#include <lotto/util/macros.h>

#define FOR_EACH_ENFORCE_MODE                                                  \
    GEN_ENFORCE_MODE(PC)                                                       \
    GEN_ENFORCE_MODE(CAT)                                                      \
    GEN_ENFORCE_MODE(TID)                                                      \
    GEN_ENFORCE_MODE(ADDRESS)                                                  \
    GEN_ENFORCE_MODE(DATA)                                                     \
    GEN_ENFORCE_MODE(CUSTOM)                                                   \
    GEN_ENFORCE_MODE(SEED)

#define GEN_ENFORCE_MODE(mode) ENFORCE_MODE_NUM_##mode,
enum enforce_mode_num_t {
    ENFORCE_MODE_NUM_NONE = 0,
    FOR_EACH_ENFORCE_MODE ENFORCE_MODE_NUM_END_,
    ENFORCE_MODE_ANY = ~0
};
#undef GEN_ENFORCE_MODE

#define GEN_ENFORCE_MODE(mode)                                                 \
    ENFORCE_MODE_##mode = NUM_TO_BIT(ENFORCE_MODE_NUM_##mode),
enum enforce_mode {
    ENFORCE_MODE_NONE = 0,
    FOR_EACH_ENFORCE_MODE ENFORCE_MODE_END_,
    MODE_FORCE_SIZE_ = (1ULL << 62),
};
#undef GEN_ENFORCE_MODE

#define ENFORCE_MODES_MAX_LEN 256

typedef uint64_t enforce_modes_t;

#define ENFORCE_MODES_DEFAULT ENFORCE_MODE_NONE

typedef struct enforce_config {
    marshable_t m;
    enforce_modes_t modes;
    bool compare_address;
} enforce_config_t;

#define ENFORCE_DATA_SIZE 64

typedef struct enforce_state {
    marshable_t m;
    context_t ctx;
    arg_t val;
    clk_t clk;
    stable_address_t pc;
    uint64_t seed;
    char data[ENFORCE_DATA_SIZE];
} enforce_state_t;

/**
 * Writes the given enforcement mode set `modes` into `dst` string.
 *
 * @param modes enforcement mode set
 * @param dst char array of at least ENFORCE_MODES_MAX_LEN bytes
 */
void enforce_modes_str(enforce_modes_t modes, char *dst);

/**
 * Returns the enforcement mode set from `src`.
 *
 * @param src string of the form `VAL|VAL|...`.
 * @return change point set
 */
enforce_modes_t enforce_modes_from(const char *src);

/**
 * Returns whether the set contains the given enforcement mode.
 *
 * @param enforce_modes enforcement mode set
 * @param enforce_mode enforcement mode
 * @return true if enforce_mode in enforce_modes, otherwise false
 */
bool enforce_modes_has(enforce_modes_t enforce_modes,
                       enum enforce_mode enforce_mode);

/**
 * Returns the enforcement handler config.
 */
enforce_config_t *enforce_config();

/**
 * Returns the enforcement handler state.
 */
enforce_state_t *enforce_state();

#endif
