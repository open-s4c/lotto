#include <lotto/engine/pubsub.h>
#include <lotto/modules/enforce/events.h>

LOTTO_ADVERTISE_TYPE(EVENT_ENFORCE_VIOLATED)

PS_SUBSCRIBE(CHAIN_DICE_CONTROL, EVENT_DICE_INIT, {})
