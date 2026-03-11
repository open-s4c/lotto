#include <lotto/core/module.h>
#include <lotto/engine/pubsub.h>

LOTTO_MODULE_INIT()

/* The CLI/driver and runtime needs the engine state registration pass before it
 * marshals START and CONFIG records. The registration needs to be defered until
 * the runtime or driver trigger their respective INIT events.
 */
static void
engine_register()
{
    LOTTO_PUBLISH_CONTROL(EVENT_ENGINE__INIT);
}

LOTTO_SUBSCRIBE_CONTROL(EVENT_RUNTIME__INIT, { engine_register(); })
LOTTO_SUBSCRIBE_CONTROL(EVENT_DRIVER__INIT, { engine_register(); })
