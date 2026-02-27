#include <math.h>
#define LOGGER_BLOCK LOGGER_CUR_BLOCK

#include <lotto/brokers/pubsub.h>
#include <lotto/cli/flagmgr.h>
#include <lotto/modules/pct/state.h>

NEW_CALLBACK_FLAG(PCT_K, "k", "pct-k", "INT", "PCT k parameter", flag_uval(10),
                  {
                      if (!pct_config()->initd)
                          pct_config()->k = as_uval(v);
                      pct_config()->initd = true;
                  })
NEW_CALLBACK_FLAG(PCT_D, "d", "pct-d", "INT", "PCT d parameter", flag_uval(3), {
    pct_config()->d     = as_uval(v);
    pct_config()->chpts = pct_config()->d - 1;
})
PS_SUBSCRIBE_INTERFACE(TOPIC_AFTER_UNMARSHAL_FINAL, {
    uint64_t old_k  = pct_config()->k;
    uint64_t new_k  = pct_state()->counts.kmax;
    pct_config()->k = ceil(0.125 * (double)new_k + 0.875 * (double)old_k);
})
