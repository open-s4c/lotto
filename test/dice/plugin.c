#ifdef TEST_LATE
    #define DICE_MODULE_SLOT SLOT_LATE
    #define TEST_EVENT ANY_EVENT
    #define TEST_HIT   'D'
#else
    #define DICE_MODULE_SLOT SLOT_EARLY
    #define TEST_EVENT EVENT_TEST
    #define TEST_HIT   'B'
#endif

#include "trace.h"

PS_SUBSCRIBE(CHAIN_DICE_CONTROL, EVENT_DICE_INIT,
             { record(initialization, TEST_HIT); })
PS_SUBSCRIBE(CHAIN_TEST, TEST_EVENT, { record(delivery, TEST_HIT); })
