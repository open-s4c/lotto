#define DICE_MODULE_SLOT SLOT_SECOND
#include "trace.h"

char initialization[16];
char delivery[16];
unsigned replies;

/* One fixed default plus explicit overrides in reverse dispatch order.
 * Only SLOT_FIRST may publish replies, so route checks also test the override.
 */
PS_SUBSCRIBE(CHAIN_DICE_CONTROL, EVENT_DICE_INIT,
             { record(initialization, 'C'); })
PS_SUBSCRIBE_SLOT(CHAIN_DICE_CONTROL, EVENT_DICE_INIT, SLOT_FIRST,
                  { record(initialization, 'A'); })

PS_SUBSCRIBE(CHAIN_TEST, EVENT_TEST, { record(delivery, 'C'); })
PS_SUBSCRIBE_SLOT(CHAIN_TEST, EVENT_TEST, SLOT_FIRST, {
    assert(!strcmp(initialization, "ABCDE"));
    record(delivery, 'A');
    PS_PUBLISH(CHAIN_REPLY, EVENT_REPLY, NULL, md);
})
PS_SUBSCRIBE(CHAIN_REPLY, EVENT_REPLY, { replies++; })
