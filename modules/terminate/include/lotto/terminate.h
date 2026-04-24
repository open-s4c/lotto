/**
 * @file terminate.h
 * @brief Semantic termination interface.
 */
#ifndef LOTTO_TERMINATE_H
#define LOTTO_TERMINATE_H

#include <lotto/qemu/lotto_udf.h>

typedef enum terminate_kind {
    TERMINATE_SUCCESS = 0,
    TERMINATE_FAILURE,
    TERMINATE_ABANDON,
} terminate_t;

static inline void
lotto_terminate(terminate_t kind)
{
    switch (kind) {
        case TERMINATE_SUCCESS:
            LOTTO_EXIT_SUCCESS;
            break;
        case TERMINATE_FAILURE:
            LOTTO_EXIT_FAILURE;
            break;
        case TERMINATE_ABANDON:
            LOTTO_EXIT_ABANDON;
            break;
        default:
            break;
    }
}

#endif
