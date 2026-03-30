#include <lotto/engine/pubsub.h>
#include <lotto/engine/sequencer.h>
#include <lotto/modules/rusty/events.h>

void lotto_rust_subscribe();
void lotto_rust_publish_execute(const capture_point *ctx,
                                const capture_point *event);
void lotto_rust_publish_arrival(const capture_point *ctx,
                                sequencer_decision *e);
void lotto_rust_after_unmarshal_config(void);
void lotto_rust_after_unmarshal_persistent(void);
void lotto_rust_after_unmarshal_final(void);
void lotto_rust_before_marshal_config(void);
void lotto_rust_before_marshal_persistent(void);
void lotto_rust_before_marshal_final(void);
void lotto_rust_register();
void lotto_rust_init();

LOTTO_ADVERTISE_TYPE(EVENT_AWAIT)
LOTTO_ADVERTISE_TYPE(EVENT_SPIN_START)
LOTTO_ADVERTISE_TYPE(EVENT_SPIN_END)

static void
_rusty_capture_handle(const capture_point *ctx, event_t *e)
{
    lotto_rust_publish_arrival(ctx, e);
}

ON_SEQUENCER_CAPTURE(_rusty_capture_handle)

LOTTO_SUBSCRIBE_SEQUENCER_RESUME(ANY_EVENT, {
    lotto_rust_publish_execute((const capture_point *)md,
                               (const capture_point *)event);
})
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
