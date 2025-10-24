/*
 */
#ifndef LOTTO_STATE_SEQUENCER_H
#define LOTTO_STATE_SEQUENCER_H

#include <lotto/base/marshable.h>
#include <lotto/base/record_granularity.h>
#include <lotto/base/stable_address.h>

#define STRATEGY_LEN 128

typedef struct sequencer_config {
    marshable_t m;
    record_granularities_t gran;
    uint64_t slack;
    stable_address_method_t stable_address_method;
    char strategy[STRATEGY_LEN];
} sequencer_config_t;

sequencer_config_t *sequencer_config();

#endif
