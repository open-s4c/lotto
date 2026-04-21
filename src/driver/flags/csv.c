#include <lotto/driver/flags/csv.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/string.h>
#include <lotto/sys/stdlib.h>
#include <string.h>

int
csv_for_each(const char *csv, csv_token_f *fn, void *arg)
{
    ASSERT(fn != NULL);

    if (csv == NULL || csv[0] == '\0') {
        return 0;
    }

    char *copy = sys_strdup(csv);
    ASSERT(copy != NULL);

    for (char *tok = strtok(copy, ","); tok != NULL; tok = strtok(NULL, ",")) {
        int r = fn(tok, arg);
        if (r != 0) {
            sys_free(copy);
            return r;
        }
    }

    sys_free(copy);
    return 0;
}
