#include <lotto/driver/events.h>
#include <lotto/engine/pubsub.h>
#include <lotto/util/once.h>

#define LOTTO_PHASE_XTOR_PRIO 1001

void ps_init_();

static void
start_driver_registration_(void)
{
    once({
        ps_init_();
        START_REGISTRATION_PHASE();
        START_INITIALIZATION_PHASE();
        LOTTO_PUBLISH_CONTROL(EVENT_DRIVER__REGISTER_FLAGS);
        LOTTO_PUBLISH_CONTROL(EVENT_DRIVER__REGISTER_COMMANDS);
    });
}

static void __attribute__((constructor(LOTTO_PHASE_XTOR_PRIO)))
lotto_driver_start_phases_(void)
{
    start_driver_registration_();
}
