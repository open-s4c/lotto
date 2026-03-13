#include <lotto/engine/pubsub.h>

void lotto_rust_cli_init();
void lotto_rust_cli_register_flags();

LOTTO_SUBSCRIBE_CONTROL(EVENT_LOTTO_REGISTER,
                        { lotto_rust_cli_register_flags(); })
LOTTO_SUBSCRIBE_CONTROL(EVENT_LOTTO_REGISTER, { lotto_rust_cli_init(); })
