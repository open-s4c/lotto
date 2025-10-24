/*
 */
#define LOG_PREFIX LOG_CUR_FILE
#define LOG_BLOCK  LOG_CUR_BLOCK

#include <lotto/brokers/pubsub.h>
#include <lotto/cli/flagmgr.h>
#include <lotto/states/handlers/ichpt.h>

NEW_PRETTY_CALLBACK_FLAG(HANDLER_ICHPT_ENABLED, "", "handler-ichpt",
                         "enable ichpt handler", flag_on(), STR_CONVERTER_BOOL,
                         { ichpt_config()->enabled = is_on(v); })
PS_SUBSCRIBE_INTERFACE(TOPIC_AFTER_UNMARSHAL_FINAL, {
    bag_clear(ichpt_initial());
    bag_copy(ichpt_initial(), ichpt_final());
})
