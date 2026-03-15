#include <stdbool.h>
#include <stdio.h>

#include <dice/pubsub.h>
#include <lotto/core/module.h>
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
LOTTO_ADVERTISE_TYPE(EVENT_LOTTO_REGISTER)
LOTTO_ADVERTISE_TYPE(EVENT_LOTTO_INIT)

LOTTO_MODULE_INIT({
    startup_barrier_closed_ = true;
    START_REGISTRATION_PHASE();
    START_INITIALIZATION_PHASE();
})
