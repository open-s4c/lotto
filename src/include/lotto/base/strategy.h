/*
 */
#ifndef SCHEDULER_STRATEGY_H
#define SCHEDULER_STRATEGY_H

typedef enum strategy {
    STRATEGY_RANDOM,
    STRATEGY_PCT,
    STRATEGY_POS,
    STRATEGY_STATIC,
} strategy_t;

#define N_STRAT 3

const char *strategy_str(strategy_t strategy);

strategy_t strategy_from(const char *strategy);

#endif
