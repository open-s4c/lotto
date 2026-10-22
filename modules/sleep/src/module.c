#include <lotto/engine/pubsub.h>
#include <lotto/modules/sleep/events.h>

LOTTO_ADVERTISE_TYPE(EVENT_SLEEP_YIELD)

PS_SUBSCRIBE(CHAIN_DICE_CONTROL, EVENT_DICE_INIT, {})
