#include <lotto/engine/dispatcher.h>
#include <lotto/engine/pubsub.h>

void lotto_rust_subscribe();
void lotto_rust_publish_execute(const struct value *v);
void lotto_rust_publish_arrival(const context_t *ctx, event_t *e);
void lotto_rust_after_unmarshal_config(void);
void lotto_rust_after_unmarshal_persistent(void);
void lotto_rust_after_unmarshal_final(void);
void lotto_rust_before_marshal_config(void);
void lotto_rust_before_marshal_persistent(void);
void lotto_rust_before_marshal_final(void);
void lotto_rust_register();
void lotto_rust_init();

static void
_rusty_capture_handle(const context_t *ctx, event_t *e)
{
    lotto_rust_publish_arrival(ctx, e);
}

REGISTER_HANDLER(_rusty_capture_handle)

LOTTO_SUBSCRIBE(EVENT_ENGINE__NEXT_TASK, { lotto_rust_publish_execute(&v); })
LOTTO_SUBSCRIBE(EVENT_ENGINE__AFTER_UNMARSHAL_CONFIG,
                { lotto_rust_after_unmarshal_config(); })
LOTTO_SUBSCRIBE(EVENT_ENGINE__AFTER_UNMARSHAL_PERSISTENT,
                { lotto_rust_after_unmarshal_persistent(); })
LOTTO_SUBSCRIBE(EVENT_ENGINE__AFTER_UNMARSHAL_FINAL,
                { lotto_rust_after_unmarshal_final(); })
LOTTO_SUBSCRIBE(EVENT_ENGINE__BEFORE_MARSHAL_CONFIG,
                { lotto_rust_before_marshal_config(); })
LOTTO_SUBSCRIBE(EVENT_ENGINE__BEFORE_MARSHAL_PERSISTENT,
                { lotto_rust_before_marshal_persistent(); })
LOTTO_SUBSCRIBE(EVENT_ENGINE__BEFORE_MARSHAL_FINAL,
                { lotto_rust_before_marshal_final(); })

static void DICE_CTOR
rust_subscribe_()
{
    lotto_rust_subscribe();
}

/* Rust runtime follows Lotto's explicit two-phase startup contract. */
ON_REGISTRATION_PHASE({ lotto_rust_register(); })
ON_INITIALIZATION_PHASE({ lotto_rust_init(); })
