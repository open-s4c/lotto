#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <lotto/sys/ensure.h>
#include <lotto/sys/modules.h>

#ifdef __APPLE__
    #define SUFFIX ".dylib"
#else
    #define SUFFIX ".so"
#endif

static int
count_module(module_t *module, void *arg)
{
    size_t *count = arg;
    ENSURE(module->name != NULL);
    ENSURE(module->path != NULL);
    ENSURE(!module->shadowed && !module->disabled);
    (*count)++;
    return 0;
}

int
main(void)
{
    char directory[] = "/tmp/lotto-modules-XXXXXX";
    ENSURE(mkdtemp(directory) != NULL);
    const char *prefixes[] = {
        "lotto-driver-",
        "lotto-runtime-",
        "lotto-runtime-dbg-",
    };
    char filename[256];
    for (unsigned i = 0; i < 64; i++) {
        for (unsigned p = 0; p < 3; p++) {
            snprintf(filename, sizeof(filename), "%s/%sprobe%u" SUFFIX,
                     directory, prefixes[p], i);
            FILE *file = fopen(filename, "w");
            ENSURE(file != NULL);
            ENSURE(fclose(file) == 0);
        }
    }
    // Exercise growth beyond the old 100-entry limit and reuse after clearing.
    for (unsigned i = 0; i < 2; i++) {
        lotto_module_scan(directory, NULL, NULL);
        size_t count = 0;
        ENSURE(lotto_module_foreach(count_module, &count) == 0);
        ENSURE(count == 192);
        lotto_module_clear();
    }
    for (unsigned i = 0; i < 64; i++) {
        for (unsigned p = 0; p < 3; p++) {
            snprintf(filename, sizeof(filename), "%s/%sprobe%u" SUFFIX,
                     directory, prefixes[p], i);
            ENSURE(unlink(filename) == 0);
        }
    }
    ENSURE(rmdir(directory) == 0);
    return 0;
}
