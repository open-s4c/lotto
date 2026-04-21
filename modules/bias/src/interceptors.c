#include <dice/chains/intercept.h>
#include <dice/module.h>
#include <dice/pubsub.h>
#include <lotto/bias.h>
#include <lotto/modules/bias/events.h>

void
lotto_bias_policy(bias_policy_t policy)
{
    bias_policy_event_t ev = {.policy = policy};
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_BIAS_POLICY, &ev, 0);
}

void
lotto_bias_toggle(void)
{
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_BIAS_TOGGLE, NULL, 0);
}
