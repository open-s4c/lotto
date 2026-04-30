#ifndef LOTTO_MODULES_SLEEP_STATE_H
#define LOTTO_MODULES_SLEEP_STATE_H

#include <stdbool.h>
#include <stdint.h>

#include <lotto/base/marshable.h>

typedef enum sleep_mode {
    SLEEP_MODE_ONCE,
    SLEEP_MODE_UNTIL,
} sleep_mode_t;

typedef struct sleep_config {
    marshable_t m;
    bool enabled;
    sleep_mode_t mode;
} sleep_config_t;

const char *sleep_mode_str(uint64_t mode);
uint64_t sleep_mode_from(const char *mode);
void sleep_mode_all_str(char *str);
sleep_config_t *sleep_config(void);

#endif
