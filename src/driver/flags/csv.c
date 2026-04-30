#include <string.h>

#include <lotto/driver/flags/csv.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/stdlib.h>
#include <lotto/sys/string.h>

int
csv_for_each(const char *csv, csv_token_f *fn, void *arg)
{
    ASSERT(fn != NULL);

    if (csv == NULL || csv[0] == '\0') {
        return 0;
    }

    char *copy = sys_strdup(csv);
    ASSERT(copy != NULL);

    char *saveptr = NULL;
    for (char *tok = strtok_r(copy, ",", &saveptr); tok != NULL;
         tok       = strtok_r(NULL, ",", &saveptr)) {
        int r = fn(tok, arg);
        if (r != 0) {
            sys_free(copy);
            return r;
        }
    }

    sys_free(copy);
    return 0;
}
