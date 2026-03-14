#include <lotto/engine/pubsub.h>

void lotto_rust_register();
void lotto_rust_init();

LOTTO_SUBSCRIBE_CONTROL(EVENT_LOTTO_REGISTER, { lotto_rust_register(); })
LOTTO_SUBSCRIBE_CONTROL(EVENT_LOTTO_INIT, { lotto_rust_init(); })
