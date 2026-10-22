#ifndef LOTTO_DICE_H
#define LOTTO_DICE_H

#include <lotto/dice-config.h>

#ifdef __cplusplus
extern "C" {
#endif

int lotto_subscribe(const char *configuration, chain_id chain, type_id type,
                    ps_callback_f callback, int slot);
void lotto_close_subscriptions(void);

#ifdef __cplusplus
}
#endif

#undef ps_subscribe
#define ps_subscribe(CHAIN, TYPE, CALLBACK, SLOT)                              \
    lotto_subscribe(DICE_CONFIG_ID, CHAIN, TYPE, CALLBACK, SLOT)
#endif
