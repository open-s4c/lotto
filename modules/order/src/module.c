#include <lotto/engine/pubsub.h>
#include <lotto/modules/order/events.h>

LOTTO_ADVERTISE_TYPE(EVENT_ORDER_SET)

PS_SUBSCRIBE(CHAIN_DICE_CONTROL, EVENT_DICE_INIT, {})
