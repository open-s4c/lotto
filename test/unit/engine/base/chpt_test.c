#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <lotto/base/chpt.h>

typedef struct {
    const char *value;
    chpts_t result;
} test_case_t;

int
main(void)
{
    test_case_t test_case[] = {

        {"YIELD", CHPT_YIELD},
        {"ATOMIC", CHPT_ATOMIC},
        {"WATCHDOG", CHPT_WATCHDOG},
        {"INSTRUCT", CHPT_INSTRUCT},
        {"ATOMIC|WATCHDOG", CHPT_ATOMIC | CHPT_WATCHDOG},
        {"YIELD|INSTRUCT|ORDER", CHPT_INSTRUCT | CHPT_YIELD | CHPT_USER_ORDER},

        NULL};
    test_case_t *tc = test_case;

    for (; tc->value; tc++) {
        printf("[%s]\n", tc->value);
        chpts_t res = chpts_from(tc->value);
        char val[CHPTS_MAX_LEN];
        chpts_str(res, val);

        printf("\t--> %s\n", val);
        assert(res == tc->result);
        assert(strcmp(val, tc->value) == 0);
    }
    return 0;
}
