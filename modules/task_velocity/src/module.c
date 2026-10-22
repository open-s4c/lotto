#include <lotto/engine/pubsub.h>
#include <lotto/modules/task_velocity/events.h>

LOTTO_ADVERTISE_TYPE(EVENT_TASK_VELOCITY)

PS_SUBSCRIBE(CHAIN_DICE_CONTROL, EVENT_DICE_INIT, {})
