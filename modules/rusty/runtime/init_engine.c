#include <lotto/engine/pubsub.h>

void lotto_rust_subscribe();
void lotto_rust_register();
void lotto_rust_init();

static void DICE_CTOR
rust_subscribe_()
{
    lotto_rust_subscribe();
}

/* Rust runtime follows Lotto's explicit two-phase startup contract. */
ON_REGISTRATION_PHASE({ lotto_rust_register(); })
ON_INITIALIZATION_PHASE({ lotto_rust_init(); })
