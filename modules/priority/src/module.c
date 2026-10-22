#include <lotto/engine/pubsub.h>
#include <lotto/modules/priority/events.h>

LOTTO_ADVERTISE_TYPE(EVENT_PRIORITY_SET)

PS_SUBSCRIBE(CHAIN_DICE_CONTROL, EVENT_DICE_INIT, {})
