/*
 */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "scheme.h"
#include <chibi/eval.h>
#include <chibi/sexp.h>
#include <lotto/base/flags.h>
#include <lotto/base/value.h>
#include <lotto/cli/subcmd.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/stdio.h>
#include <lotto/util/macros.h>
#include <lotto/util/once.h>

#define SEXP_ALWAYS_SUCCEED(val)                                               \
    ({                                                                         \
        sexp tmp = val;                                                        \
        if (sexp_exceptionp(tmp)) {                                            \
            sys_fprintf(stdout, "SEXP exception on line %d: %s\n", __LINE__,   \
                        sexp_string_data(sexp_exception_message(tmp)));        \
            ASSERT(0);                                                         \
        }                                                                      \
        tmp;                                                                   \
    })

static sexp global_ctx;
static args_t global_args;
static flags_t *global_flags;

static void register_lotto_ffi();

int
lotto_scheme_eval_file(const char *fname)
{
    once(register_lotto_ffi());

    sexp env = SEXP_ALWAYS_SUCCEED(sexp_context_env(global_ctx));

    SEXP_ALWAYS_SUCCEED(sexp_load(
        global_ctx, SEXP_ALWAYS_SUCCEED(sexp_c_string(global_ctx, fname, -1)),
        env));

    return 0;
}

static sexp
flags_set_wrapper(sexp ctx, sexp self, sexp_sint_t n, sexp f, sexp val)
{
    ASSERT(n == 2 && "Internal scheme bug - this condition must not happen");

    sexp_assert_type(ctx, sexp_integerp, SEXP_FIXNUM, f);

    flag_t c_f = (flag_t)sexp_unbox_fixnum(f);

    enum value_type type = flags_get_type(global_flags, c_f);
    struct value flag_val;

    switch (type) {
        case VALUE_TYPE_BOOL:
            sexp_assert_type(ctx, sexp_booleanp, SEXP_BOOLEAN, val);
            flag_val = bval(sexp_unbox_boolean(val));
            break;
        case VALUE_TYPE_UINT64:
            sexp_assert_type(ctx, sexp_integerp, SEXP_FIXNUM, val);
            flag_val = uval(sexp_unbox_fixnum(val));
            break;
        case VALUE_TYPE_STRING:
            sexp_assert_type(ctx, sexp_stringp, SEXP_STRING, val);
            flag_val = sval(sexp_string_data(val));
            break;
        default:
            ASSERT(0 && "not supported");
            __builtin_unreachable();
    }

    flags_set_by_opt(global_flags, c_f, flag_val);

    sexp scheme_result = SEXP_ALWAYS_SUCCEED(sexp_make_fixnum(0));

    return scheme_result;
}

static void
register_flags_set(sexp ctx)
{
    sexp env  = SEXP_ALWAYS_SUCCEED(sexp_context_env(ctx));
    sexp name = SEXP_ALWAYS_SUCCEED(sexp_c_string(ctx, "flags_set", -1));
    sexp proc = SEXP_ALWAYS_SUCCEED(sexp_define_foreign(
        ctx, env, "flags_set", 2, (sexp_proc1)flags_set_wrapper));
    SEXP_ALWAYS_SUCCEED(sexp_env_define(ctx, env, name, proc));

    for (const option_t *o = flagmgr_options() + 1; o->flag != 0; o++) {
        char buf[128];
        sys_snprintf(buf, 128, "(define FLAG_%s %d)", o->name, o->flag);
    }
}


static int
count_args(char *cmd)
{
    int count   = 0;
    bool in_arg = false;
    char *it    = cmd;

    while (*it) {
        if (isspace(*it)) {
            in_arg = false;
        } else {
            if (!in_arg) {
                count++;
                in_arg = true;
            }
        }
        it++;
    }

    return count;
}

static void
parse_cmd(char *cmd, int *argc, char ***argv)
{
    int n       = count_args(cmd);
    char **args = malloc((n + 1) * sizeof(char *));

    ASSERT(args != NULL);

    int i       = 0;
    bool in_arg = false;
    char *it    = cmd;
    char *start = NULL;

    while (*it) {
        if (isspace(*it)) {
            if (in_arg) {
                *it       = '\0';
                args[i++] = start;
                in_arg    = false;
            }
        } else {
            if (!in_arg) {
                start  = it;
                in_arg = true;
            }
        }
        it++;
    }

    if (in_arg) {
        args[i++] = start;
    }

    args[i] = NULL;

    *argc = n;
    *argv = args;
}

static sexp
cmd_set(sexp ctx, sexp self, sexp_sint_t n, sexp cmd)
{
    ASSERT(n == 1 && "Internal scheme bug - this condition must not happen");

    sexp_assert_type(ctx, sexp_stringp, SEXP_STRING, cmd);

    char *c_cmd = strdup(sexp_string_data(cmd));
    static char **argv;
    static int argc = 0;

    parse_cmd(c_cmd, &argc, &argv);

    global_args = ARGS(argc, argv);

    sexp scheme_result = SEXP_ALWAYS_SUCCEED(sexp_make_fixnum(0));

    return scheme_result;
}

static void
register_cmd_set(sexp ctx)
{
    sexp env  = SEXP_ALWAYS_SUCCEED(sexp_context_env(ctx));
    sexp name = SEXP_ALWAYS_SUCCEED(sexp_c_string(ctx, "cmd_set", -1));
    sexp proc = SEXP_ALWAYS_SUCCEED(
        sexp_define_foreign(ctx, env, "cmd_set", 1, (sexp_proc1)cmd_set));
    SEXP_ALWAYS_SUCCEED(sexp_env_define(ctx, env, name, proc));
}

static sexp
lotto_cmd(sexp ctx, sexp self, sexp_sint_t n, sexp subcmd)
{
    ASSERT(n == 1 && "Internal scheme bug - this condition must not happen");

    sexp_assert_type(ctx, sexp_stringp, SEXP_STRING, subcmd);

    char *c_subcmd = sexp_string_data(subcmd);
    int res        = 0;

    subcmd_t *s = NULL;
    while ((s = subcmd_next(s))) {
        if (strcmp(c_subcmd, s->name) == 0) {
            flags_t *flags = s->defaults();
            (void)flags;
            for (flag_t b = 1; b < flags->nflags; b++) {
                if (global_flags->values[b].is_default) {
                    global_flags->values[b] = flags->values[b];
                }
            }

            res = s->func(&global_args, global_flags);
            break;
        }
    }

    sexp scheme_result = SEXP_ALWAYS_SUCCEED(sexp_make_fixnum(res));

    return scheme_result;
}

static void
register_lotto_cmd(sexp ctx)
{
    sexp env  = SEXP_ALWAYS_SUCCEED(sexp_context_env(ctx));
    sexp name = SEXP_ALWAYS_SUCCEED(sexp_c_string(ctx, "lotto", -1));
    sexp proc = SEXP_ALWAYS_SUCCEED(
        sexp_define_foreign(ctx, env, "lotto", 1, (sexp_proc1)lotto_cmd));
    SEXP_ALWAYS_SUCCEED(sexp_env_define(ctx, env, name, proc));

    subcmd_t *s = NULL;
    while ((s = subcmd_next(s))) {
        if (strcmp("-", s->name) == 0) {
            continue;
        }

        char buf[128];
        sys_snprintf(buf, 128, "(define %s \"%s\")", s->name, s->name);
        SEXP_ALWAYS_SUCCEED(sexp_eval_string(global_ctx, buf, -1, env));
    }
}

static void
register_lotto_ffi()
{
    global_flags = flagmgr_flags_alloc();
    flags_cpy(global_flags, flags_default());

    SEXP_ALWAYS_SUCCEED(sexp_load_standard_env(global_ctx, NULL, SEXP_SEVEN));
    SEXP_ALWAYS_SUCCEED(
        sexp_load_standard_ports(global_ctx, NULL, stdin, stdout, stderr, 1));

    register_flags_set(global_ctx);
    register_cmd_set(global_ctx);
    register_lotto_cmd(global_ctx);
}

void
lotto_scheme_register_library_path(const char *dir)
{
    SEXP_ALWAYS_SUCCEED(sexp_add_module_directory(
        global_ctx, SEXP_ALWAYS_SUCCEED(sexp_c_string(global_ctx, dir, -1)),
        SEXP_NULL));
}

static void LOTTO_CONSTRUCTOR
init()
{
    sexp_scheme_init();

    global_ctx =
        SEXP_ALWAYS_SUCCEED(sexp_make_eval_context(NULL, NULL, NULL, 0, 0));
}

static void LOTTO_DESTRUCTOR
fini()
{
    sexp_destroy_context(global_ctx);
}
