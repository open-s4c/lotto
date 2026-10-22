#include <lotto/engine/pubsub.h>
#include <lotto/modules/clock/events.h>

LOTTO_ADVERTISE_TYPE(EVENT_CLOCK_READ)
PS_SUBSCRIBE(CHAIN_DICE_CONTROL, EVENT_DICE_INIT, {})
