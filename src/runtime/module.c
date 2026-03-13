#include <lotto/core/module.h>
#include <lotto/engine/pubsub.h>

PS_ADVERTISE_CHAIN(CHAIN_LOTTO_CONTROL)
PS_ADVERTISE_CHAIN(CHAIN_LOTTO_DEFAULT)
LOTTO_ADVERTISE_TYPE(EVENT_RUNTIME__INIT)

void runtime_init();

LOTTO_MODULE_INIT({
    LOTTO_PUBLISH_CONTROL(EVENT_RUNTIME__INIT);
    runtime_init();
})
