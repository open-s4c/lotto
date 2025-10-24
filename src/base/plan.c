/*
 */
#include <stddef.h>
#include <stdint.h>

#include <lotto/base/plan.h>
#include <lotto/sys/assert.h>

enum action
plan_next(plan_t p)
{
    for (unsigned i = 1; i < ACTION_END_; i <<= 1)
        if (p.actions & i)
            return i;
    return ACTION_NONE;
}

bool
plan_done(plan_t *p)
{
    enum action a = plan_next(*p);
    p->actions &= ~(unsigned)a;
    return p->actions == ACTION_NONE;
}


void
plan_print(plan_t p)
{
    log_debugln("type = 0x%x next = %lu", p.actions, p.next);
}

bool
plan_has(plan_t p, enum action a)
{
    return (p.actions & a) == a;
}

const char *_action_to_str[] = {
    [ACTION_NONE]     = "NONE",     //
    [ACTION_WAKE]     = "WAKE",     //
    [ACTION_CALL]     = "CALL",     //
    [ACTION_BLOCK]    = "BLOCK",    //
    [ACTION_RETURN]   = "RETURN",   //
    [ACTION_YIELD]    = "YIELD",    //
    [ACTION_RESUME]   = "RESUME",   //
    [ACTION_CONTINUE] = "CONTINUE", //
    [ACTION_SNAPSHOT] = "SNAPSHOT", //
    [ACTION_SHUTDOWN] = "SHUTDOWN", //
};

const char *
action_str(action_t a)
{
    ASSERT(a < ACTION_END_);
    return _action_to_str[a];
}
