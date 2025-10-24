/*
 */
#ifndef LOTTO_ORDER_H
#define LOTTO_ORDER_H
/**
 * @file order.h
 * @brief The ordering points interface.
 *
 * Ordering points are events which must be executed in the order defined by
 * their arguments. Orders start with 1, must be unique, and must not contain
 * gaps. When an out of order point is about to be executed, the task is blocked
 * until the previous point is executed.
 */

#include <stdbool.h>
#include <stdint.h>

#define LOTTO_ORDER(order, ...)                                                \
    {                                                                          \
        uint64_t ghost __attribute__((__cleanup__(lotto_order_cleanup))) =     \
            order;                                                             \
        (void)ghost;                                                           \
        lotto_order(2 * order - 1);                                            \
        __VA_ARGS__;                                                           \
    }

/**
 * Inserts an ordering point.
 *
 * @param order order
 */
void lotto_order(uint64_t order) __attribute__((weak));
/**
 * Inserts an ordering point if the condition is satisfied.
 *
 * @param cond  condition
 * @param order order
 */
void lotto_order_cond(bool cond, uint64_t order) __attribute__((weak));

static inline void __attribute__((unused)) lotto_order_cleanup(uint64_t *order)
{
    lotto_order(2 * *order);
}

#endif
