/*
 */
#ifndef LOTTO_STATE_ICHPT_H
#define LOTTO_STATE_ICHPT_H
#include <stdbool.h>

#include <lotto/base/vec.h>
#include <lotto/base/marshable.h>
#include <lotto/base/stable_address.h>

typedef struct ichpt_config {
    marshable_t m;
    bool enabled;
} ichpt_config_t;

typedef struct {
    vecitem_t t;
    stable_address_t addr;
} item_t;

ichpt_config_t *ichpt_config();
vec_t *ichpt_initial();
vec_t *ichpt_final();
int ichpt_item_compare(const vecitem_t *, const vecitem_t *);

#endif
