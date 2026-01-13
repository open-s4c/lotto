/*
 */
#define LOGGER_PREFIX LOGGER_CUR_FILE
#define LOGGER_BLOCK  LOGGER_CUR_BLOCK
#include <lotto/cli/flagmgr.h>
#include <lotto/states/prng.h>
#include <lotto/sys/assert.h>
NEW_PUBLIC_CALLBACK_FLAG(SEED, "", "seed", "INT",
                         "seed for pseudo-random number generator",
                         flag_uval_force_opt(UINT64_MAX),
                         { prng()->seed = (uint32_t)as_uval(v); })
FLAG_GETTER(seed, SEED)
