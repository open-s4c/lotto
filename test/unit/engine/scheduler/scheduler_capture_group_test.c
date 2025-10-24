#include <assert.h>
#include <stdlib.h>

#include <engine/scheduler/scheduler_capture_group.h>
#include <lotto/base/context.h>
#include <lotto/cli/envvar.h>

#define MAX_ENVVARS       3
#define MAX_INITIAL_TASKS 2
#define MAX_TEST_STEPS    5

typedef struct {
    scheduler_param_t input_param;
    scheduler_param_t output_param;
} test_step_t;

typedef struct {
    test_step_t steps[MAX_TEST_STEPS];
    envvar_t vars[MAX_ENVVARS];
    task_id initial_tasks[MAX_INITIAL_TASKS];
} test_t;
void
handler_register(scheduler_interface_t *scheduler_ptr, handler_type type)
{
}
bool
scheduler_param_equals(scheduler_param_t *x, scheduler_param_t *y)
{
    unsigned int size_x = scheduler_task_size(x->tasks);
    unsigned int size_y = scheduler_task_size(y->tasks);
    if (size_x != size_y) {
        return false;
    }
    for (int i = 1; i < size_x; i++) {
        if (!scheduler_task_contains(y->tasks, x->tasks[i])) {
            return false;
        }
    }
    for (int i = 0; i < CTX_NARGS; i++) {
        if (x->ctx->args[i].width == CTX_ARG_EMPTY) {
            if (y->ctx->args[i].width == CTX_ARG_EMPTY) {
                break;
            } else {
                return false;
            }
        }
        if ((x->ctx->args[i].width == CTX_ARG_PTR)) {
            if (y->ctx->args[i].width != CTX_ARG_PTR) {
                return false;
            }
        } else if (argmatch(x->ctx->args[i], x->ctx->args[i])) {
            continue;
        } else {
            return false;
        }

        if (x->ctx->args[i].value.ptr != y->ctx->args[i].value.ptr &&
            strcmp((char *)x->ctx->args[i].value.ptr,
                   (char *)y->ctx->args[i].value.ptr) != 0) {
            return false;
        }
    }
    // NOLINTBEGIN(bugprone-suspicious-string-compare,
    // bugprone-suspicious-memory-comparison)
    return !(memcmp(&x->action, &y->action, sizeof(action_t)) ||
             memcmp(&x->opts, &y->opts, sizeof(sched_opts)) ||
             memcmp(&x->ctx->cat, &y->ctx->cat, sizeof(category_t)) ||
             memcmp(&x->ctx->id, &y->ctx->id, sizeof(task_id)) ||
             memcmp(&x->ctx->pc, &y->ctx->pc, sizeof(uintptr_t)));
    // NOLINTEND(bugprone-suspicious-string-compare,
    // bugprone-suspicious-memory-comparison)
}

void
reset_env(const char **vars)
{
    for (int i = 0; vars[i]; i++) {
        unsetenv(vars[i]);
    }
}

void
register_tasks(scheduler_interface_t *scheduler, const task_id *tasks)
{
    for (int i = 0; tasks[i]; i++) {
        scheduler->task_register(tasks[i]);
    }
}

test_t *test_cases;

int
main()
{
    const char *vars[] = {"LOTTO_DELAYED_FUNCTIONS", "LOTTO_DELAYED_CALLS",
                          NULL};
    test_t tests[]     = {
            {.vars          = {{"LOTTO_DELAYED_FUNCTIONS", .sval = ""},
                               {"LOTTO_DELAYED_CALLS", .uval = 1},
                               NULL},
             .initial_tasks = {1, NO_TASK},
             .steps         = {{.input_param  = {.ctx   = ctx(.id = 1, .cat = CAT_CALL,
                                                              .args = {ctx_arg_ptr(NULL),
                                                                       ctx_arg_ptr("foo")}),
                                                 .tasks = ((task_id[]){1, NO_TASK})},
                                .output_param = {.ctx   = ctx(.id = 1, .cat = CAT_CALL,
                                                              .args = {ctx_arg_ptr(NULL),
                                                                       ctx_arg_ptr("foo")}),
                                                 .tasks = ((task_id[]){1, NO_TASK})}},
                               NULL}},
            NULL};
    test_cases = malloc(sizeof(tests));
    memcpy(test_cases, tests, sizeof(tests));
    for (int i = 0; test_cases[i].steps[0].input_param.ctx; i++) {
        reset_env(vars);
        scheduler_capture_group.reset(true);
        register_tasks(&scheduler_capture_group, test_cases[i].initial_tasks);
        for (int j = 0; test_cases[i].steps[j].input_param.ctx; j++) {
            scheduler_capture_group.next(&test_cases[i].steps[j].input_param);
            assert(
                scheduler_param_equals(&test_cases[i].steps[j].input_param,
                                       &test_cases[i].steps[j].output_param));
        }
    }
}
