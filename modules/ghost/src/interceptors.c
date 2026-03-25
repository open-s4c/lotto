#include "events.h"
#include <dice/chains/intercept.h>
#include <dice/module.h>
#include <dice/pubsub.h>
#include <lotto/unsafe/ghost.h>

void
_lotto_ghost_enter()
{
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_GHOST_START, 0, 0);
}

void
_lotto_ghost_leave()
{
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_GHOST_END, 0, 0);
}
