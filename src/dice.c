#include <lotto/dice.h>

static bool subscriptions_closed_;

void
lotto_close_subscriptions(void)
{
    subscriptions_closed_ = true;
}

int
lotto_subscribe(const char *configuration, chain_id chain, type_id type,
                ps_callback_f callback, int slot)
{
    if (subscriptions_closed_)
        log_fatal(
            "late subscription after EVENT_LOTTO_REGISTER: "
            "chain=%u type=%u slot=%d",
            chain, type, slot);
    return dice_subscribe_abi(configuration, chain, type, callback, slot);
}
