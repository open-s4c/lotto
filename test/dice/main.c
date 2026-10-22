#define DICE_MODULE_SLOT SLOT_PUBLISHER
#include <dlfcn.h>
#include <stdio.h>

#include "trace.h"

int
main(int argc, char **argv)
{
    /* Neither linked subscriptions nor plugin constructors initialize early. */
    assert(!initialization[0]);
    if (argc == 4)
        lotto_close_subscriptions();
    for (int i = 1; i < argc && i <= 2; i++) {
        void *handle = dlopen(argv[i], RTLD_NOW | RTLD_LOCAL);
        if (!handle)
            fprintf(stderr, "%s\n", dlerror());
        assert(handle);
        assert(!initialization[0]);
    }
    assert(ps_publish(CHAIN_TEST, EVENT_TEST, NULL, NULL) == PS_OK);
    assert(!strcmp(initialization, "ABCDE"));
    assert(!strcmp(delivery, "ABCDE"));
    assert(replies == 1);
    delivery[0] = 0;
    assert(ps_publish(CHAIN_TEST, EVENT_TEST, NULL, NULL) == PS_OK);
    assert(!strcmp(initialization, "ABCDE"));
    assert(!strcmp(delivery, "ABCDE"));
    assert(replies == 2);
    return 0;
}
