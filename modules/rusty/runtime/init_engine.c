#include <lotto/engine/pubsub.h>

void lotto_rust_engine_init();

LOTTO_SUBSCRIBE_CONTROL(EVENT_LOTTO_REGISTER, { lotto_rust_engine_init(); })
