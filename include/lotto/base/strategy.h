/**
 * @file strategy.h
 * @brief Base declarations for strategy.
 */
#ifndef LOTTO_BASE_STRATEGY_H
#define LOTTO_BASE_STRATEGY_H

typedef enum strategy {
    STRATEGY_RANDOM,
    STRATEGY_PCT,
    STRATEGY_POS,
    STRATEGY_FIRST,
} strategy_t;

#define N_STRAT 4

const char *strategy_str(strategy_t strategy);

strategy_t strategy_from(const char *strategy);

#endif
