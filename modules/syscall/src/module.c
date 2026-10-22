#include <lotto/engine/pubsub.h>
#include <lotto/modules/syscall/events.h>

LOTTO_ADVERTISE_TYPE(EVENT_SYSCALL)

PS_SUBSCRIBE(CHAIN_DICE_CONTROL, EVENT_DICE_INIT, {})
