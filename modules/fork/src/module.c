#include <lotto/engine/pubsub.h>
#include <lotto/modules/fork/events.h>

LOTTO_ADVERTISE_TYPE(EVENT_FORK_EXECVE)

PS_SUBSCRIBE(CHAIN_DICE_CONTROL, EVENT_DICE_INIT, {})
