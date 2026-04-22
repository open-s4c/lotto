/**
 * @file ingress.h
 * @brief Runtime declarations for ingress.
 */
#ifndef LOTTO_INGRESS_H
#define LOTTO_INGRESS_H

#include <dice/pubsub.h>
#include <dice/events/pthread.h>
#include <lotto/base/task_id.h>
#include <lotto/runtime/capture_point.h>
#include <lotto/runtime/ingress_events.h>
#include <lotto/runtime/mediator.h>
#include <lotto/util/macros.h>

/* Runtime ingress pubsub chains. */
#define CHAIN_INGRESS_EVENT  9
#define CHAIN_INGRESS_BEFORE 10
#define CHAIN_INGRESS_AFTER  11

#define FORWARD_CAPTURE_TO_INGRESS(SUFFIX, TYPE)                               \
    static void __attribute__((constructor(DICE_XTOR_PRIO)))                   \
    lotto_advertise_type_##TYPE##_(void)                                       \
    {                                                                          \
        ps_register_type(TYPE, #TYPE);                                         \
    }                                                                          \
    PS_SUBSCRIBE(CAPTURE_##SUFFIX, TYPE, {                                     \
        capture_point *cp = EVENT_PAYLOAD(event);                              \
        PS_PUBLISH(CHAIN_INGRESS_##SUFFIX, TYPE, cp, md);                      \
        return PS_STOP_CHAIN;                                                  \
    })

/*******************************************************************************
 * mediator's task_id (in TLS)
 ******************************************************************************/

static inline task_id
get_task_id(void)
{
    mediator_t *m = mediator_get(NULL, false);
    ASSERT((!!m) && "thread-specific mediator data not initialized");
    return (m->id);
}

/*
 * Return the mediator of the current task, init flag also initializes
 * mediator if not yet.
 */
mediator_t *get_mediator(bool new_task);

/*
 * Like get_mediator but does not initialize mediator if not yet.
 */
mediator_t *get_existing_mediator();

typedef void (*fini_t)();
void lotto_intercept_register_fini(fini_t func);
void lotto_intercept_fini();

#endif
