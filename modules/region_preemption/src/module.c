#include <lotto/engine/pubsub.h>
#include <lotto/modules/region_preemption/events.h>

LOTTO_ADVERTISE_TYPE(EVENT_REGION_PREEMPTION)

PS_SUBSCRIBE(CHAIN_DICE_CONTROL, EVENT_DICE_INIT, {})
