#define DICE_MODULE_SLOT SLOT_LEGACY
#include "trace.h"

/* The old interface coexists with explicit subscriptions. */
DICE_MODULE_INIT({ record(initialization, 'E'); })
PS_SUBSCRIBE(CHAIN_TEST, EVENT_TEST, { record(delivery, 'E'); })
