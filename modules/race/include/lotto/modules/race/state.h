#ifndef LOTTO_STATE_RACE_H
#define LOTTO_STATE_RACE_H
#include <stdbool.h>

#include <lotto/base/marshable.h>

#define RACE_DEFAULT

typedef struct race_config {
    marshable_t m;
    bool enabled;
    bool ignore_write_write;
    bool only_write_ichpt;
    bool abort_on_race;
} race_config_t;

race_config_t *race_config();

#endif
