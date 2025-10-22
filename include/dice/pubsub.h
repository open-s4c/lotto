/*
 * Copyright (C) 2023-2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: 0BSD
 * -----------------------------------------------------------------------------
 * @file pubsub.h
 * @brief Interruptable chain-based pubsub
 *
 * This component provides a chain-based pubsub. When an event is published to a
 * chain (aka topic), handlers are called in priority order. Each handler has
 * the option of interruping the chain by returning false.
 *
 * Events are have a type (`type_id`) and have a payload `payload`. A chain is
 * identified by a `chain_id`.
 *
 * There are two ways how handlers can be called:
 * - callback function pointers registered with ps_subscribe calls, see below.
 * - dispatch calls, having predefined name and compilation, see dispatch.h.
 *
 * ## Naming
 *
 * For a lack of better name, this component is called `pubsub`. It combines
 * aspects of several GoF design patterns such as pubsub, observer, and
 * chain-of-responsibility pattern. However, it violates some requirements of
 * each aspect such that no name seem to perfectly match it.
 */
#ifndef DICE_PUBSUB_H
#define DICE_PUBSUB_H
#include <stdbool.h>
#include <stdint.h>

#include <dice/compiler.h>
#include <dice/log.h>
#include <dice/types.h>
#include <vsync/atomic.h>
#include <vsync/atomic/internal/macros.h>

/* Return values of ps_publish. */
enum ps_err {
    PS_OK          = 0,
    PS_STOP_CHAIN  = 1,
    PS_DROP_EVENT  = 2,
    PS_HANDLER_OFF = 3,
    PS_INVALID     = -1,
    PS_ERROR       = -2,
};

/* ps_callback_f is the subscriber interface.
 *
 * Callback handlers can return the following codes:
 * - PS_OK: event handled successfully
 * - PS_STOP_CHAIN: event handled successfully, but chain should be
 *   interrupted
 * - PS_DROP_EVENT: event not handled and chain should be interrupted
 * - PS_HANDLER_OFF: handler is disabled
 */
typedef enum ps_err (*ps_callback_f)(const chain_id, const type_id, void *event,
                                     metadata_t *md);

/* ps_publish publishes an event to a chain.
 *
 * Event `type` defines the type of the `event` parameter, `chain` determines in
 * which chain the event is published and defines the subtype of `md`. The
 * metadata `md` can be NULL.
 *
 * Returns one of the PS_ error codes above.
 */
enum ps_err ps_publish(const chain_id chain, const type_id type, void *event,
                       metadata_t *md);


/* PS_PUBLISH simplifies the publication and drop mechanism of metadata. */
#define PS_PUBLISH(chain, type, event, md)                                     \
    do {                                                                       \
        metadata_t __md = {0};                                                 \
        metadata_t *_md = (md) != NULL ? (metadata_t *)(md) : &__md;           \
        if (_md->drop)                                                         \
            break;                                                             \
        enum ps_err err = ps_publish(chain, type, event, md);                  \
        if (err == PS_DROP_EVENT)                                              \
            _md->drop = true;                                                  \
        if (err != PS_OK && err != PS_DROP_EVENT)                              \
            log_fatal("could not publish");                                    \
    } while (0)

/* ps_subscribe subscribes a callback in a chain for an event.
 *
 * `prio` determines the relative order in which the callbacks are called with
 * published events.
 *
 * Note: ps_subscribe should only be called during initialization of the
 * system.
 *
 * Returns 0 if success, otherwise non-zero.
 *
 * Note: Instead of directly using this function, use `PS_SUBSCRIBE()` macro
 * defined in `dice/module.h` header file.
 */
int ps_subscribe(chain_id chain, type_id type, ps_callback_f cb, int prio);

/* EVENT_PAYLOAD casts the event argument `event` to type of the given
 * variable.
 *
 * This macro is intended to be used with PS_SUBSCRIBE. The user must
 * know the type of the argument and then the following pattern can be used:
 *
 *     PS_SUBSCRIBE(SOME_HOOK, SOME_EVENT, {
 *         some_known_type *ev = EVENT_PAYLOAD(ev);
 *         ...
 *         })
 */
#define EVENT_PAYLOAD(var) (__typeof(var))event

#define INTERPOSE_PC                                                           \
    (__builtin_extract_return_addr(__builtin_return_address(0)))

#endif /* DICE_PUBSUB_H */
