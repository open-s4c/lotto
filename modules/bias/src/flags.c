#include "state.h"
#include <lotto/driver/flagmgr.h>

static void
_bias_policy_help(char *dst)
{
    bias_policy_all_str(dst);
}

NEW_PRETTY_CALLBACK_FLAG(
    BIAS_POLICY_FLAG, "", "bias-policy", "set scheduler bias policy",
    flag_uval(BIAS_POLICY_NONE),
    STR_CONVERTER_GET(bias_policy_str, bias_policy_from, 8, _bias_policy_help),
    {
        bias_config()->initial_policy = (bias_policy_t)as_uval(v);
        if (!bias_config()->toggle_explicit) {
            bias_config()->toggle_policy =
                bias_config()->initial_policy == BIAS_POLICY_NONE ?
                    BIAS_POLICY_CURRENT :
                    BIAS_POLICY_NONE;
        }
    })

NEW_PRETTY_CALLBACK_FLAG(BIAS_TOGGLE_FLAG, "", "bias-toggle",
                         "set scheduler bias toggle policy",
                         flag_uval(BIAS_POLICY_CURRENT),
                         STR_CONVERTER_GET(bias_policy_str, bias_policy_from, 8,
                                           _bias_policy_help),
                         {
                             bias_config()->toggle_policy =
                                 (bias_policy_t)as_uval(v);
                             bias_config()->toggle_explicit = true;
                         })
