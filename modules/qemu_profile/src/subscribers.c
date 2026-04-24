#include "profile_emit.h"
#include <dice/pubsub.h>
#include <lotto/engine/pubsub.h>
#include <lotto/modules/qemu/events.h>
#include <lotto/modules/yield/events.h>
#include <lotto/runtime/events.h>
#include <lotto/runtime/trap.h>

PS_SUBSCRIBE(CHAIN_QEMU_CONTROL, EVENT_QEMU_INIT, { qemu_profile_on_init(); })

PS_SUBSCRIBE(CHAIN_QEMU_CONTROL, EVENT_QEMU_FINI, { qemu_profile_on_fini(); })

PS_SUBSCRIBE(CHAIN_QEMU_CONTROL, EVENT_QEMU_TRANSLATE, {
    const qemu_translate_event_t *ev = data;
    qemu_profile_on_translate(ev->insn_count);
})

PS_SUBSCRIBE(INTERCEPT_EVENT, EVENT_MA_READ,
             { qemu_profile_on_memaccess(false); })

PS_SUBSCRIBE(INTERCEPT_BEFORE, EVENT_MA_AREAD,
             { qemu_profile_on_memaccess(false); })

PS_SUBSCRIBE(INTERCEPT_EVENT, EVENT_MA_WRITE,
             { qemu_profile_on_memaccess(true); })

PS_SUBSCRIBE(INTERCEPT_BEFORE, EVENT_MA_AWRITE,
             { qemu_profile_on_memaccess(true); })

PS_SUBSCRIBE(CHAIN_LOTTO_TRAP, EVENT_YIELD_USER,
             { qemu_profile_on_udf_yield(); })
