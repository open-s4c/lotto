#define LOGGER_PREFIX LOGGER_CUR_FILE
#define LOGGER_BLOCK  LOGGER_CUR_BLOCK

#include <stdint.h>
#include <string.h>

#include <lotto/base/record_granularity.h>
#include <lotto/base/stable_address.h>
#include <lotto/driver/flagmgr.h>
#include <lotto/driver/flags/prng.h>
#include <lotto/driver/flags/sequencer.h>
#include <lotto/engine/state.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/string.h>

#ifndef DEFAULT_SLACK_TIME
    #define DEFAULT_SLACK_TIME 10
#endif

static void
_record_granularities_help(char *dst)
{
    record_granularities_str(RECORD_GRANULARITY_ANY, dst);
}

static void
_stable_address_method_help(char *dst)
{
    stable_address_method_all_str(dst);
}

NEW_PUBLIC_CALLBACK_FLAG(SEED, "", "seed", "INT",
                         "seed for pseudo-random number generator",
                         flag_uval_force_opt(UINT64_MAX),
                         { prng()->seed = (uint32_t)as_uval(v); })

NEW_PUBLIC_PRETTY_CALLBACK_FLAG(
    RECORD_GRANULARITY, "", "record-granularity", "record granularity",
    flag_uval(RECORD_GRANULARITIES_DEFAULT),
    STR_CONVERTER_PRINT(record_granularities_str, record_granularities_from,
                        RECORD_GRANULARITIES_MAX_LEN,
                        _record_granularities_help),
    { sequencer_config()->gran = as_uval(v); })

NEW_CALLBACK_FLAG(SLACK, "", "slack", "INT", "slack time in milliseconds",
                  flag_uval(DEFAULT_SLACK_TIME),
                  { sequencer_config()->slack = as_uval(v); })

NEW_PUBLIC_CALLBACK_FLAG(STRATEGY, "s", "strategy", "STRAT",
                         "select strategy pct, pos, or random",
                         flag_sval("pos"), {
                             ASSERT(sys_strlen(as_sval(v)) < STRATEGY_LEN);
                             strcpy(sequencer_config()->strategy, as_sval(v));
                         })

NEW_PUBLIC_PRETTY_CALLBACK_FLAG(
    STABLE_ADDRESS_METHOD, "", "stable-address-method",
    "method to produce addresses stable accross runs",
    flag_uval(STABLE_ADDRESS_METHOD_NONE),
    STR_CONVERTER_GET(stable_address_method_str, stable_address_method_from,
                      STABLE_ADDRESS_MAX_STR_LEN, _stable_address_method_help),
    { sequencer_config()->stable_address_method = as_uval(v); })

FLAG_GETTER(seed, SEED)
FLAG_GETTER(record_granularity, RECORD_GRANULARITY)
FLAG_GETTER(strategy, STRATEGY)
FLAG_GETTER(stable_address_method, STABLE_ADDRESS_METHOD)
