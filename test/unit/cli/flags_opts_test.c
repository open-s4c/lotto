#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <lotto/base/tidset.h>
#include <lotto/cli/flagmgr.h>
#include <lotto/cli/flags/sequencer.h>

#define TEST_CASE(name, expected, ...)                                         \
    {                                                                          \
        name, expected, .sel = { __VA_ARGS__ }                                 \
    }

struct test_case {
    const char *name;
    const char *expected;
    flag_t sel[MAX_FLAGS];
};

tidset_t *
get_available_tasks()
{
    return NULL;
}

struct option long_opts[MAX_FLAGS] = {0};

DECLARE_FLAG_INPUT;
DECLARE_FLAG_OUTPUT;
DECLARE_FLAG_ROUNDS;
DECLARE_FLAG_VERBOSE;

int
main()
{
    struct test_case tc[] = {
        TEST_CASE("INPUT", "i:", FLAG_INPUT, FLAG_SEL_SENTINEL),
        TEST_CASE("OUTPUT", "o:", FLAG_OUTPUT, FLAG_SEL_SENTINEL),
        TEST_CASE("INPUT|OUTPUT", "i:o:", FLAG_INPUT, FLAG_OUTPUT,
                  FLAG_SEL_SENTINEL),
        TEST_CASE("ROUNDS", "r:", FLAG_ROUNDS, FLAG_SEL_SENTINEL),
        TEST_CASE("VERBOSE", "v", FLAG_VERBOSE, FLAG_SEL_SENTINEL),
        TEST_CASE("VERBOSE|STRATEGY", "s:v", flag_strategy(), FLAG_VERBOSE,
                  FLAG_SEL_SENTINEL),
        NULL,
    };
    struct test_case *nxt = tc;
    for (; nxt->name; nxt++) {
        char opts[1024] = {0};
        flags_opts(opts, false, nxt->sel, long_opts);
        printf("%s\t== %s\n", opts, nxt->name);
        assert(strcmp(nxt->expected, opts) == 0);
    }
    return 0;
}
