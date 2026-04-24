/**
 * @file terminate.h
 * @brief Semantic termination interface.
 */
#ifndef LOTTO_TERMINATE_H
#define LOTTO_TERMINATE_H

#include <lotto/base/reason.h>
#include <lotto/modules/terminate/events.h>
#include <lotto/runtime/capture_point.h>
#include <lotto/runtime/runtime.h>
#include <lotto/qemu/lotto_udf.h>

typedef enum terminate_kind {
    TERMINATE_SUCCESS = 0,
    TERMINATE_FAILURE,
    TERMINATE_ABANDON,
} terminate_t;

static inline void
qlotto_terminate(terminate_t kind)
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

static inline void
lotto_terminate(terminate_t kind)
{
#if defined(__aarch64__)
    qlotto_terminate(kind);
#else
    capture_point cp = {.func = __FUNCTION__};
    reason_t reason  = REASON_SUCCESS;

    switch (kind) {
        case TERMINATE_SUCCESS:
            cp.type_id = EVENT_TERMINATE_SUCCESS;
            reason     = REASON_SUCCESS;
            break;
        case TERMINATE_FAILURE:
            cp.type_id = EVENT_TERMINATE_FAILURE;
            reason     = REASON_ASSERT_FAIL;
            break;
        case TERMINATE_ABANDON:
            cp.type_id = EVENT_TERMINATE_ABANDON;
            reason     = REASON_SUCCESS;
            break;
        default:
            return;
    }

    lotto_exit(&cp, reason);
#endif
}

#endif
