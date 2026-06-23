#include "mock-probe.h"
#include <dlfcn.h>

typedef int (*probe_count_fn_t)(void);

static int
probe_count_(const char *symbol)
{
    probe_count_fn_t fn = (probe_count_fn_t)dlsym(RTLD_DEFAULT, symbol);
    return fn ? fn() : 0;
}

int
lotto_test_handler_count(void)
{
    return probe_count_("lotto_test_handler_count_impl");
}

int
lotto_test_malloc_before_count(void)
{
    return probe_count_("lotto_test_malloc_before_count_impl");
}

int
lotto_test_malloc_after_count(void)
{
    return probe_count_("lotto_test_malloc_after_count_impl");
}

int
lotto_test_malloc_segfault_before_count(void)
{
    return probe_count_("lotto_test_malloc_segfault_before_count_impl");
}

int
lotto_test_malloc_segfault_after_count(void)
{
    return probe_count_("lotto_test_malloc_segfault_after_count_impl");
}
