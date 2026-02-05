#ifdef DICE_MODULE_SLOT
    #undef DICE_MODULE_SLOT
#endif
#define DICE_MODULE_SLOT 10

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define LOGGER_PREFIX LOGGER_CUR_FILE
#define LOGGER_BLOCK  LOGGER_CUR_BLOCK
#include <lotto/base/marshable.h>
#include <lotto/brokers/pubsub.h>
#include <lotto/brokers/statemgr.h>
#include <lotto/engine/prng.h>
#include <lotto/states/prng.h>
#include <lotto/sys/logger.h>
#include <lotto/util/casts.h>
#include <lotto/util/once.h>

#define PRNG_MASK 0xFFFFFFFF

static prng_t _prng;
REGISTER_CONFIG(_prng, { logger_infof("seed = %u\n", _prng.seed); })
PS_SUBSCRIBE_INTERFACE(TOPIC_AFTER_UNMARSHAL_CONFIG, {
    logger_debugf("seed = %u\n", _prng.seed);
    const char *var = getenv("LOTTO_SEED");
    if (var) {
        _prng.seed = atoi(var);
        logger_debugf("Seed from envvar: %u\n", _prng.seed);
    }
})

prng_t *
prng()
{
    return &_prng;
}

uint64_t
prng_seed()
{
    return _prng.seed;
}

// actually only returns  4 random bytes (32bit) due to & PRNG_MASK
uint64_t
prng_next()
{
    return (CAST_TYPE(uint64_t, rand_r(&_prng.seed)) & PRNG_MASK);
}

uint64_t
prng_range(uint64_t min, uint64_t max)
{
    ASSERT(max > min);
    uint64_t v     = prng_next();
    uint64_t range = max - min;
    uint64_t rn    = min + (v % range);
    return rn;
}

double
prng_real()
{
    double rn = 1.0 * (double)prng_next() / RAND_MAX;
    return rn;
}
