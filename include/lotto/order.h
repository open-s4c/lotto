#ifndef LOTTO_ORDER_H
#define LOTTO_ORDER_H

#include <stdbool.h>
#include <stdint.h>

void lotto_order(uint64_t order) __attribute__((weak));
void lotto_order_cond(bool cond, uint64_t order) __attribute__((weak));

#define LOTTO_ORDER(order, ...)                                                \
    {                                                                          \
        uint64_t _lotto_order_id __attribute__((__cleanup__(lotto_order_cleanup))) = \
            (order);                                                           \
        (void)_lotto_order_id;                                                 \
        lotto_order(2 * (order)-1);                                            \
        __VA_ARGS__;                                                           \
    }

static inline void __attribute__((unused))
lotto_order_cleanup(uint64_t *order)
{
    lotto_order(2 * *order);
}

static inline void
lotto_order_if(bool cond, uint64_t order)
{
    if (cond) {
        lotto_order(order);
    }
}

#endif /* LOTTO_ORDER_H */
