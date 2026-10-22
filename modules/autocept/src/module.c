#include <lotto/engine/pubsub.h>
#include <lotto/modules/autocept/events.h>

LOTTO_ADVERTISE_TYPE(EVENT_AUTOCEPT_CALL)

PS_SUBSCRIBE(CHAIN_DICE_CONTROL, EVENT_DICE_INIT, {})
/* Standalone fixtures also use the legacy dispatcher. */
PS_DISPATCH_SLOT_ON(DICE_MODULE_SLOT)
