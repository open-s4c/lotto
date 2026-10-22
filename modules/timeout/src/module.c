#include <lotto/engine/pubsub.h>
#include <lotto/modules/timeout/timeout.h>

LOTTO_ADVERTISE_TYPE(EVENT_TIMEOUT_TRIGGER)

PS_SUBSCRIBE(CHAIN_DICE_CONTROL, EVENT_DICE_INIT, {})
