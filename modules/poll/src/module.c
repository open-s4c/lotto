#include "poll.h"
#include <lotto/engine/pubsub.h>
#include <lotto/modules/poll/events.h>

LOTTO_ADVERTISE_TYPE(EVENT_POLL)

PS_SUBSCRIBE(CHAIN_DICE_CONTROL, EVENT_DICE_INIT, {})
