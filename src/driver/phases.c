#include <lotto/driver/events.h>
#include <lotto/engine/pubsub.h>
#include <lotto/util/once.h>

static void
start_driver_registration_(void)
{
    once({
        LOTTO_PUBLISH_CONTROL(EVENT_DRIVER__REGISTER_FLAGS);
        LOTTO_PUBLISH_CONTROL(EVENT_DRIVER__REGISTER_COMMANDS);
    });
}

static void __attribute__((constructor))
lotto_driver_start_phases_(void)
{
    start_driver_registration_();
}
