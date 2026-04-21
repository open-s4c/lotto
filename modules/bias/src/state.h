#ifndef LOTTO_STATE_BIAS_H
#define LOTTO_STATE_BIAS_H

#include <stdbool.h>

#include <lotto/base/marshable.h>
#include <lotto/bias.h>

typedef struct bias_config {
    marshable_t m;
    bias_policy_t initial_policy;
    bias_policy_t toggle_policy;
    bool toggle_explicit;
} bias_config_t;

typedef struct bias_state {
    marshable_t m;
    bias_policy_t current_policy;
    bias_policy_t toggle_policy;
} bias_state_t;

bias_config_t *bias_config(void);
bias_state_t *bias_state(void);
const char *bias_policy_str(uint64_t bits);
uint64_t bias_policy_from(const char *src);
void bias_policy_all_str(char *dst);

#endif
