
#include <lotto/driver/flagmgr.h>
#include <lotto/driver/flags/modules.h>
#include <lotto/engine/pubsub.h>
#include <lotto/modules/ichpt/state.h>

static void
_ichpt_enabled(bool enabled)
{
    ichpt_config()->enabled = enabled;
}

_FLAGMGR_SUBSCRIBE({ register_runtime_switchable_module(LOTTO_MODULE_NAME,
                                                        _ichpt_enabled); })

NEW_PRETTY_CALLBACK_FLAG(HANDLER_ICHPT_ENABLED, "", "handler-ichpt",
                         "enable ichpt handler", flag_on(), STR_CONVERTER_BOOL,
                         { ichpt_config()->enabled = is_on(v); })
LOTTO_SUBSCRIBE(EVENT_ENGINE__AFTER_UNMARSHAL_FINAL, {
    vec_union(ichpt_initial(), ichpt_final(), ichpt_item_compare);
})
