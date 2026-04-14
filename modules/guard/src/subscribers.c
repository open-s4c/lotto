#include <stdbool.h>
#include <stdio.h>

#include <dice/chains/intercept.h>
#include <dice/module.h>
#include <lotto/engine/events.h>
#include <lotto/engine/pubsub.h>
#include <lotto/sys/abort.h>
#include <lotto/sys/logger.h>

static bool ready_;
DICE_HIDE bool
guard_ready_(void)
{
    return ready_;
}

DICE_WEAK int
ps_subscribe_(chain_id chain, type_id type, ps_callback_f cb, int slot)
{
    return PS_OK;
}
int
ps_subscribe(chain_id chain, type_id type, ps_callback_f cb, int slot)
{
    if (guard_ready_()) {
        logger_fatalf(
            "late subscription after EVENT_LOTTO_REGISTER: chain=%u type=%u "
            "slot=%d\n",
            (unsigned int)chain, (unsigned int)type, slot);
    }
    return ps_subscribe_(chain, type, cb, slot);
}

LOTTO_SUBSCRIBE_CONTROL(EVENT_LOTTO_REGISTER, { ready_ = true; })

PS_SUBSCRIBE(INTERCEPT_BEFORE, ANY_EVENT,
             { return guard_ready_() ? PS_OK : PS_STOP_CHAIN; })

PS_SUBSCRIBE(INTERCEPT_AFTER, ANY_EVENT,
             { return guard_ready_() ? PS_OK : PS_STOP_CHAIN; })

PS_SUBSCRIBE(INTERCEPT_EVENT, ANY_EVENT,
             { return guard_ready_() ? PS_OK : PS_STOP_CHAIN; })
