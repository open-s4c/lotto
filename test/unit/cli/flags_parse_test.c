#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lotto/base/tidset.h>
#include <lotto/cli/args.h>
#include <lotto/cli/flagmgr.h>
#include <lotto/cli/flags/prng.h>
#include <lotto/cli/flags/sequencer.h>
#include <lotto/states/handlers/enforce.h>
#include <lotto/states/handlers/termination.h>
#include <lotto/sys/ensure.h>

#define MAX_ARGS 128

struct test_case {
    const char *command[MAX_ARGS];
    flag_t sel[MAX_FLAGS];
    int ret;
    flags_t *flags;
};

struct flag_value_pair {
    flag_t flag;
    struct flag_val value;
};

void
enforce_modes_str(enforce_modes_t modes, char *str)
{
}

enforce_modes_t
enforce_modes_from(const char *str)
{
    return 0;
}

const char *
termination_mode_str(termination_mode_t mode)
{
    return NULL;
}

termination_mode_t
termination_mode_from(const char *src)
{
    return 0;
}

tidset_t *
get_available_tasks()
{
    return NULL;
}

void
termination_mode_all_str(char *dst)
{
}

void
crep_truncate(clk_t clk)
{
}

flags_t *
with_flags(struct flag_value_pair flags[])
{
    flags_t *ret = flagmgr_flags_alloc();
    flags_cpy(ret, flags_default());
    for (int i = 0; flags[i].flag; i++) {
        struct value val = flags[i].value._val;
        if (value_type(val) != VALUE_TYPE_NONE)
            flags_set_by_opt(ret, flags[i].flag, val);
    }
    return ret;
}

#define with_flags(...) with_flags((struct flag_value_pair[]){__VA_ARGS__})

DECLARE_FLAG_HELP;
DECLARE_FLAG_INPUT;
DECLARE_FLAG_OUTPUT;
DECLARE_FLAG_VERBOSE;
DECLARE_FLAG_ROUNDS;

int
main()
{
    struct test_case tc[] = {
        {
            {"hallo"},
            {FLAG_SEL_SENTINEL},
            FLAGS_PARSE_OK,
            with_flags(FLAG_NONE),
        },
        {
            {"hello", "-h"},
            {FLAG_INPUT, FLAG_HELP, FLAG_SEL_SENTINEL},
            FLAGS_PARSE_HELP,
        },
        {
            {"hello", "-v"},
            {FLAG_VERBOSE, FLAG_SEL_SENTINEL},
            FLAGS_PARSE_OK,
            with_flags({FLAG_VERBOSE, flag_on()}, {FLAG_NONE}),
        },
        {
            {"hello", "-s", "random", "-r", "10", "-i", "in", "-o", "out",
             "-v"},
            {FLAG_INPUT, FLAG_OUTPUT, FLAG_ROUNDS, flag_strategy(),
             FLAG_SEL_SENTINEL},
            FLAGS_PARSE_ERROR,
        },
        {
            {"hello", "-s", "random", "-r", "10", "-i", "in", "-o", "out",
             "-v"},
            {FLAG_INPUT, FLAG_OUTPUT, FLAG_ROUNDS, flag_strategy(),
             FLAG_VERBOSE, FLAG_SEL_SENTINEL},
            FLAGS_PARSE_OK,
            with_flags(                                 //
                {FLAG_VERBOSE, flag_on()},              //
                {FLAG_ROUNDS, flag_uval(10)},           //
                {FLAG_INPUT, flag_sval("in")},          //
                {FLAG_OUTPUT, flag_sval("out")},        //
                {flag_strategy(), flag_sval("random")}, //
                {FLAG_NONE}),

        },
        {
            {"hello", "-s", "random", "-k", "11"},
            {FLAG_SEL_SENTINEL},
            FLAGS_PARSE_ERROR,
        },
        {
            {"hello", "-x"},
            {flag_strategy(), FLAG_VERBOSE, FLAG_SEL_SENTINEL},
            FLAGS_PARSE_ERROR,
        },
        {{"hello", "-s", "random", "-v"},
         {FLAG_VERBOSE, flag_strategy(), FLAG_SEL_SENTINEL},
         FLAGS_PARSE_OK,
         with_flags({flag_strategy(), flag_sval("random")},
                    {FLAG_VERBOSE, flag_on()}, {FLAG_NONE})},
        {NULL},
    };

    struct test_case *nxt = tc;
    flags_t *flags        = flagmgr_flags_alloc();
    for (int c = 0; nxt->command[0] != NULL; nxt++, c++) {
        printf("[test %d] cmd:", c);
        int argc = 0;
        for (size_t i = 0; i < MAX_ARGS && nxt->command[i] != NULL;
             i++, argc++) {
            printf(" %s", nxt->command[i]);
        }
        printf("\n");
        args_t args = {.argv = (char **)nxt->command, .argc = argc};
        flags_cpy(flags, flags_default());

        /* parse flags */
        int r = flags_parse(flags, &args, false, nxt->sel);


        /* compare return value */
        ENSURE(r == nxt->ret && "parse error code mismatch");
        if (r != FLAGS_PARSE_OK)
            continue;

        printf("sizeof(enum flag) = %lu\n", sizeof(flag_t));

        /* compare all flags */
        int i = 0;
        for (flag_t _f = 1; _f < flags->nflags; _f++) {
            flag_t f = _f;
            printf("Testing flag: 0x%x (%d)\n", f, i++);
            struct flag_val exp = flags_get(nxt->flags, f);
            struct flag_val val = flags_get(flags, f);

            ENSURE(flag_type(exp) == flag_type(val));

            if (f == flag_seed()) {
                ENSURE(exp.force_no_default);
                if (exp.is_default) {
                    ENSURE(flag_as_uval(exp) != flag_as_uval(val));
                    continue;
                }
            }

            switch (flag_type(val)) {
                case VALUE_TYPE_BOOL:
                    ENSURE(flag_is_on(exp) == flag_is_on(val));
                    break;
                case VALUE_TYPE_UINT64:
                    ENSURE(flag_as_uval(exp) == flag_as_uval(val));
                    break;
                case VALUE_TYPE_STRING:
                    ENSURE(strcmp(flag_as_sval(exp), flag_as_sval(val)) == 0);
                    break;
                case VALUE_TYPE_NONE:
                    ENSURE(0 && "Flag type not initialized");
                    break;
                default:
                    ENSURE(0 && "invalid type");
            }
        }
    }
    flags_free(flags);
    return 0;
}
