#include "interceptors.h"
#include <dice/module.h>
#include <dice/pubsub.h>
#include <dice/self.h>
#include <lotto/engine/pubsub.h>
#include <lotto/modules/qemu/events.h>
#include <lotto/modules/qemu/util.h>
#include <lotto/modules/yield/events.h>
#include <lotto/qemu/lotto_udf.h>
#include <lotto/runtime/capture_point.h>
#include <lotto/runtime/events.h>
#include <lotto/qemu/trap.h>

void
emit_udf_trap(unsigned int cpu_index, void *udata)
{
    (void)udata;
    if (self_md() == NULL) {
        PS_PUBLISH(INTERCEPT_EVENT, EVENT_RUNTIME__NOP, NULL, NULL);
    }

    CPUARMState *armcpu      = qemu_get_armcpu_context(cpu_index);
    struct lotto_trap_event event = {
        .regs =
            {
                armcpu->xregs[UDF_REG_ARG_1],
                armcpu->xregs[UDF_REG_ARG_2],
                armcpu->xregs[UDF_REG_ARG_3],
                armcpu->xregs[UDF_REG_ARG_4],
                armcpu->xregs[UDF_REG_ARG_5],
            },
    };
    capture_point cp = {
        .chain_id = CHAIN_LOTTO_TRAP,
        .type_id  = (type_id)event.regs[0],
        .payload  = &event,
        .func     = __FUNCTION__,
    };

    if (cp.type_id == EVENT_QEMU_INSTRUMENT) {
        qemu_set_instrumentation_enabled(cpu_index, event.regs[1] != 0);
        return;
    }

    if (!qemu_instrumentation_enabled(cpu_index)) {
        return;
    }

    PS_PUBLISH(CHAIN_LOTTO_TRAP, cp.type_id, &cp, self_md());
}
