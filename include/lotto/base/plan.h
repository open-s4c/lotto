#ifndef LOTTO_PLAN_H
#define LOTTO_PLAN_H
#include <stdbool.h>
#include <stdint.h>

#include <lotto/base/arg.h>
#include <lotto/base/clk.h>
#include <lotto/base/reason.h>
#include <lotto/base/task_id.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger.h>
#include <lotto/sys/now.h>

#define PLAN_NARGS 4

typedef enum action {
    ACTION_NONE     = 0x000, //< do no action
    ACTION_WAKE     = 0x001, //< wake task in .next field
    ACTION_CALL     = 0x002, //< perform external call
    ACTION_BLOCK    = 0x004, //< block current task
    ACTION_RETURN   = 0x008, //< call engine_return
    ACTION_YIELD    = 0x010, //< wait for current task turn or any
    ACTION_RESUME   = 0x020, //< call engine_resume
    ACTION_CONTINUE = 0x040, //< do not action, continue running
    ACTION_SNAPSHOT = 0x080, //< stop execution and snapshot
    ACTION_SHUTDOWN = 0x100, //< stop execution
    ACTION_END_     = 0x200, //< end marker for iterations
} action_t;

typedef enum replay_type {
    REPLAY_OFF,
    REPLAY_ANY_TASK,
    REPLAY_ON,
} replay_type_t;

typedef struct plan {
    unsigned actions; //< Sequence of actions to be perfomed by the caller.
    task_id next;     //< Next task to be awaken (if applicable, ie, act
                      //  ACTION_WAKE).
    bool with_slack;  //< Should keep some slack time
    reason_t reason;  //< Reason to create a record (if applicable).
    replay_type_t replay_type; //< Whether the capture point has been replayed
                               // by the engine.
    clk_t clk;                 //< The current clock.
    bool (*any_task_filter)(
        task_id); //< returns true if the task may wake as ANY_TASK
    arg_t args[PLAN_NARGS];
} plan_t;

action_t plan_next(plan_t p);
bool plan_done(plan_t *p);
void plan_print(plan_t p);
bool plan_has(plan_t p, action_t a);

const char *action_str(action_t a);

#endif
