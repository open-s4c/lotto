#include <stdbool.h>
#include <stdio.h>

#include <dice/events/memaccess.h>
#include <dice/pubsub.h>
#include <lotto/engine/events.h>
#include <lotto/engine/pubsub.h>
#include <lotto/sys/abort.h>

static bool startup_barrier_closed_;

DICE_HIDE bool
should_intercept_malloc(void)
{
    return startup_barrier_closed_;
}

DICE_WEAK int
ps_subscribe_(chain_id chain, type_id type, ps_callback_f cb, int slot)
{
    return PS_OK;
}
int
ps_subscribe(chain_id chain, type_id type, ps_callback_f cb, int slot)
{
    if (startup_barrier_closed_) {
        fprintf(
            stderr,
            "late subscription after EVENT_LOTTO_REGISTER: chain=%u type=%u "
            "slot=%d\n",
            (unsigned int)chain, (unsigned int)type, slot);
        fflush(stderr);
        sys_abort();
    }
    return ps_subscribe_(chain, type, cb, slot);
}

PS_ADVERTISE_CHAIN(CHAIN_LOTTO_CONTROL)
PS_ADVERTISE_CHAIN(CHAIN_LOTTO_DEFAULT)
PS_ADVERTISE_CHAIN(CHAIN_INGRESS_EVENT)
PS_ADVERTISE_CHAIN(CHAIN_INGRESS_AFTER)
PS_ADVERTISE_CHAIN(CHAIN_INGRESS_BEFORE)
PS_ADVERTISE_CHAIN(CHAIN_SEQUENCER_CAPTURE)
PS_ADVERTISE_CHAIN(CHAIN_SEQUENCER_RESUME)
LOTTO_ADVERTISE_TYPE(EVENT_LOTTO_STARTUP_SYNC)
LOTTO_ADVERTISE_TYPE(EVENT_LOTTO_REGISTER)
LOTTO_ADVERTISE_TYPE(EVENT_LOTTO_INIT)
LOTTO_ADVERTISE_TYPE(EVENT_MA_READ)
LOTTO_ADVERTISE_TYPE(EVENT_MA_WRITE)
LOTTO_ADVERTISE_TYPE(EVENT_MA_AREAD)
LOTTO_ADVERTISE_TYPE(EVENT_MA_AWRITE)
LOTTO_ADVERTISE_TYPE(EVENT_MA_RMW)
LOTTO_ADVERTISE_TYPE(EVENT_MA_XCHG)
LOTTO_ADVERTISE_TYPE(EVENT_MA_CMPXCHG)
LOTTO_ADVERTISE_TYPE(EVENT_MA_CMPXCHG_WEAK)
LOTTO_ADVERTISE_TYPE(EVENT_MA_FENCE)
LOTTO_ADVERTISE_TYPE(EVENT_MA_READ_RANGE)
LOTTO_ADVERTISE_TYPE(EVENT_MA_WRITE_RANGE)

LOTTO_MODULE_INIT()

static void __attribute__((constructor)) lotto_startup_()
{
    PS_PUBLISH(CHAIN_CONTROL, EVENT_LOTTO_STARTUP_SYNC, 0, 0);
    startup_barrier_closed_ = true;
    START_REGISTRATION_PHASE();
    START_INITIALIZATION_PHASE();
}
