#include <stdint.h>

#include <lotto/base/record_granularity.h>
#include <lotto/base/stable_address.h>
#include <lotto/driver/flagmgr.h>
#include <lotto/driver/flags/prng.h>
#include <lotto/driver/flags/sequencer.h>
#include <lotto/engine/state.h>

#ifndef DEFAULT_SLACK_TIME
    #define DEFAULT_SLACK_TIME 0
#endif

static void
_record_granularities_help(char *dst)
{
    record_granularities_all_str(dst);
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
    RECORD_GRANULARITY, "", "record-granularity",
    "record granularity MINIMAL,SWITCH,CHPT,CAPTURE,ANY",
    flag_uval(RECORD_GRANULARITIES_DEFAULT),
    STR_CONVERTER_PRINT(record_granularities_str, record_granularities_from,
                        RECORD_GRANULARITIES_MAX_LEN,
                        _record_granularities_help),
    { sequencer_config()->gran = as_uval(v); })

NEW_CALLBACK_FLAG(SLACK, "k", "slack", "INT", "slack time in milliseconds",
                  flag_uval(DEFAULT_SLACK_TIME),
                  { sequencer_config()->slack = as_uval(v); })

NEW_PUBLIC_PRETTY_CALLBACK_FLAG(
    STABLE_ADDRESS_METHOD, "a", "stable-address-method",
    "method to produce addresses stable accross runs",
    flag_uval(STABLE_ADDRESS_METHOD_NONE),
    STR_CONVERTER_GET(stable_address_method_str, stable_address_method_from,
                      STABLE_ADDRESS_MAX_STR_LEN, _stable_address_method_help),
    { sequencer_config()->stable_address_method = as_uval(v); })

FLAG_GETTER(seed, SEED)
FLAG_GETTER(record_granularity, RECORD_GRANULARITY)
FLAG_GETTER(stable_address_method, STABLE_ADDRESS_METHOD)
