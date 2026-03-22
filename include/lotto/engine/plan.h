/**
 * @file plan.h
 * @brief Engine declarations for plans.
 */
#ifndef LOTTO_ENGINE_PLAN_H
#define LOTTO_ENGINE_PLAN_H

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

enum action {
    ACTION_NONE     = 0x000,
    ACTION_WAKE     = 0x001,
    ACTION_CALL     = 0x002,
    ACTION_BLOCK    = 0x004,
    ACTION_RETURN   = 0x008,
    ACTION_YIELD    = 0x010,
    ACTION_RESUME   = 0x020,
    ACTION_CONTINUE = 0x040,
    ACTION_SNAPSHOT = 0x080,
    ACTION_SHUTDOWN = 0x100,
    ACTION_END_     = 0x200,
};

typedef enum replay_type {
    REPLAY_OFF,
    REPLAY_ANY_TASK,
    REPLAY_ON,
} replay_type_t;

struct plan {
    unsigned actions;
    task_id next;
    bool with_slack;
    reason_t reason;
    replay_type_t replay_type;
    clk_t clk;
    bool (*any_task_filter)(task_id);
    arg_t args[PLAN_NARGS];
};

enum action plan_next(struct plan p);
bool plan_done(struct plan *p);
void plan_print(struct plan p);
bool plan_has(struct plan p, enum action a);

const char *action_str(enum action a);

#endif
