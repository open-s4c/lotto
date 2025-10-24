/*
 */
#include <string.h>

#include <lotto/base/strategy.h>
#include <lotto/sys/assert.h>

static const char *strategy_map[] = {
    [STRATEGY_STATIC] = "static",
    [STRATEGY_RANDOM] = "random",
    [STRATEGY_PCT]    = "pct",
    [STRATEGY_POS]    = "pos",
};

const char *
strategy_str(strategy_t strategy)
{
    return strategy_map[strategy];
}

strategy_t
strategy_from(const char *strategy)
{
    if (!strategy) {
        return STRATEGY_PCT;
    }
    for (int i = 0; i < N_STRAT; i++) {
        if (strcmp(strategy, strategy_str(i)) == 0) {
            return i;
        }
    }
    ASSERT(0 && "No strategy found");
    return 0;
}
