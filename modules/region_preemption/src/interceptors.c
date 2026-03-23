#include "state.h"
#include <dice/chains/intercept.h>
#include <dice/module.h>
#include <dice/pubsub.h>
#include <lotto/modules/region_preemption/events.h>
#include <lotto/region_preemption.h>
#include <lotto/runtime/ingress.h>

PS_ADVERTISE_TYPE(EVENT_REGION_PREEMPTION)

static void
_publish_region_preemption(bool in_region)
{
    region_preemption_event_t ev = {.in_region = in_region};
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_REGION_PREEMPTION, &ev, 0);
}

static void
intercept_region_preemption_switch(bool in_region)
{
    _publish_region_preemption(in_region);
}

void
lotto_region_preemption_switch(bool in_region)
{
    intercept_region_preemption_switch(in_region);
}

void
lotto_region_preemption_switch_cond(bool cond, bool in_region)
{
    if (!cond) {
        return;
    }
    lotto_region_preemption_switch(in_region);
}

void
lotto_region_preemption_switch_task(task_id task, bool in_region)
{
    if (get_task_id() != task) {
        return;
    }
    lotto_region_preemption_switch(in_region);
}

void
_lotto_region_atomic_enter()
{
    if (region_preemption_config()->default_on) {
        lotto_region_preemption_switch(true);
    }
}
void
_lotto_region_atomic_leave()
{
    if (region_preemption_config()->default_on) {
        lotto_region_preemption_switch(false);
    }
}
void
_lotto_region_atomic_enter_cond(bool cond)
{
    if (region_preemption_config()->default_on) {
        lotto_region_preemption_switch_cond(cond, true);
    }
}
void
_lotto_region_atomic_leave_cond(bool cond)
{
    if (region_preemption_config()->default_on) {
        lotto_region_preemption_switch_cond(cond, false);
    }
}
void
_lotto_region_atomic_enter_task(task_id task)
{
    if (region_preemption_config()->default_on) {
        lotto_region_preemption_switch_task(task, true);
    }
}
void
_lotto_region_atomic_leave_task(task_id task)
{
    if (region_preemption_config()->default_on) {
        lotto_region_preemption_switch_task(task, false);
    }
}
void
_lotto_region_nonatomic_enter()
{
    if (!region_preemption_config()->default_on) {
        lotto_region_preemption_switch(true);
    }
}
void
_lotto_region_nonatomic_leave()
{
    if (!region_preemption_config()->default_on) {
        lotto_region_preemption_switch(false);
    }
}
void
_lotto_region_nonatomic_enter_cond(bool cond)
{
    if (!region_preemption_config()->default_on) {
        lotto_region_preemption_switch_cond(cond, true);
    }
}
void
_lotto_region_nonatomic_leave_cond(bool cond)
{
    if (!region_preemption_config()->default_on) {
        lotto_region_preemption_switch_cond(cond, false);
    }
}
void
_lotto_region_nonatomic_enter_task(task_id task)
{
    if (!region_preemption_config()->default_on) {
        lotto_region_preemption_switch_task(task, true);
    }
}
void
_lotto_region_nonatomic_leave_task(task_id task)
{
    if (!region_preemption_config()->default_on) {
        lotto_region_preemption_switch_task(task, false);
    }
}
