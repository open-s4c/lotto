#include <stdlib.h>

#include <dice/module.h>
#include <lotto/base/marshable.h>
#include <lotto/engine/pubsub.h>
#include <lotto/engine/statemgr.h>
#include <lotto/engine/state.h>
#include <lotto/sys/logger.h>
#include <lotto/util/macros.h>

#define LOGGER_PREFIX LOGGER_CUR_FILE
#define LOGGER_BLOCK  LOGGER_CUR_BLOCK

extern marshable_t *catmgr_state(void) __attribute__((weak));

struct engine_state {
    prng_t prng;
    sequencer_config_t sequencer;
};

static struct engine_state _engine_state;

static void
state_prng_print(const marshable_t *m)
{
    (void)m;
    logger_infof("seed = %u\n", _engine_state.prng.seed);
}

static void
state_sequencer_print(const marshable_t *m)
{
    (void)m;
    char gran_str[RECORD_GRANULARITIES_MAX_LEN];
    record_granularities_str(_engine_state.sequencer.gran, gran_str);
    logger_infof("gran  = %s\n", gran_str);
    logger_infof("slack = %lu\n", _engine_state.sequencer.slack);
    logger_infof("stable_address_method = %lu\n",
                 _engine_state.sequencer.stable_address_method);
}

/*
 * prng, sequencer, and catmgr all register CONFIG on DICE_MODULE_SLOT.
 * statemgr binds same-slot registrations, so constructor order is irrelevant.
 */
static void LOTTO_CONSTRUCTOR
state_register_prng(void)
{
    _engine_state.prng.m = MARSHABLE_STATIC_PRINTABLE(
        sizeof(_engine_state.prng), state_prng_print);
    statemgr_register(DICE_MODULE_SLOT, &_engine_state.prng.m,
                      STATE_TYPE_CONFIG);
}

static void LOTTO_CONSTRUCTOR
state_register_sequencer(void)
{
    _engine_state.sequencer.m = MARSHABLE_STATIC_PRINTABLE(
        sizeof(_engine_state.sequencer), state_sequencer_print);
    statemgr_register(DICE_MODULE_SLOT, &_engine_state.sequencer.m,
                      STATE_TYPE_CONFIG);
}

LOTTO_SUBSCRIBE(TOPIC_AFTER_UNMARSHAL_CONFIG, {
    logger_debugf("seed = %u\n", _engine_state.prng.seed);
    const char *var = getenv("LOTTO_SEED");
    if (var) {
        _engine_state.prng.seed = atoi(var);
        logger_debugf("Seed from envvar: %u\n", _engine_state.prng.seed);
    }
})

prng_t *
prng()
{
    return &_engine_state.prng;
}

sequencer_config_t *
sequencer_config()
{
    return &_engine_state.sequencer;
}
