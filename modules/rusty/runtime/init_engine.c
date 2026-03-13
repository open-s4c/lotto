#include <lotto/core/runtime/events.h>
#include <lotto/engine/pubsub.h>

void lotto_rust_engine_init();

LOTTO_SUBSCRIBE_CONTROL(EVENT_RUNTIME__INIT, { lotto_rust_engine_init(); })
