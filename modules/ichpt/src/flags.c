
#include <lotto/driver/flagmgr.h>
#include <lotto/driver/flags/modules.h>
#include <lotto/engine/pubsub.h>
#include <lotto/modules/ichpt/state.h>

REGISTER_RUNTIME_SWITCHABLE_CONFIG(ichpt_config(), true)
LOTTO_SUBSCRIBE(EVENT_ENGINE__AFTER_UNMARSHAL_FINAL, {
    vec_union(ichpt_initial(), ichpt_final(), ichpt_item_compare);
})
