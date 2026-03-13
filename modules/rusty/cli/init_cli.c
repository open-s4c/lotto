#include <lotto/core/driver/events.h>
#include <lotto/engine/pubsub.h>

void lotto_rust_cli_init();
void lotto_rust_cli_register_flags();

LOTTO_SUBSCRIBE_CONTROL(EVENT_DRIVER__REGISTER_FLAGS,
                        { lotto_rust_cli_register_flags(); })
LOTTO_SUBSCRIBE_CONTROL(EVENT_DRIVER__INIT, { lotto_rust_cli_init(); })
