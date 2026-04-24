#include <stdbool.h>

#include "translate_aarch64.h"
#include "interceptors.h"
#include <dice/chains/intercept.h>
#include <dice/events/memaccess.h>
#include <lotto/engine/pubsub.h>
#include <lotto/modules/qemu/util.h>

static inline uint64_t
qemu_memaccess_size(qemu_plugin_meminfo_t info)
{
    return 1ull << qemu_plugin_mem_size_shift(info);
}

void
emit_ma_read(capture_point *cp, uint64_t vaddr, uint64_t size)
{
    struct ma_read_event ev = {.pc   = (const void *)cp->pc,
                               .func = cp->func,
                               .addr = (void *)vaddr,
                               .size = size};
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_MA_READ, &ev, NULL);
}

void
emit_ma_write(capture_point *cp, uint64_t vaddr, uint64_t size)
{
    struct ma_write_event ev = {.pc   = (const void *)cp->pc,
                                .func = cp->func,
                                .addr = (void *)vaddr,
                                .size = size};
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_MA_WRITE, &ev, NULL);
}

void
emit_ma_aread(capture_point *cp, uint64_t vaddr, uint64_t size)
{
    struct ma_aread_event ev = {.pc   = (const void *)cp->pc,
                                .func = cp->func,
                                .addr = (void *)vaddr,
                                .size = size};
    PS_PUBLISH(INTERCEPT_BEFORE, EVENT_MA_AREAD, &ev, NULL);
}

void
emit_ma_awrite(capture_point *cp, uint64_t vaddr, uint64_t size)
{
    struct ma_awrite_event ev = {.pc   = (const void *)cp->pc,
                                 .func = cp->func,
                                 .addr = (void *)vaddr,
                                 .size = size,
                                 .val  = {.u64 = 0}};
    PS_PUBLISH(INTERCEPT_BEFORE, EVENT_MA_AWRITE, &ev, NULL);
}

void
emit_ma_rmw(capture_point *cp, uint64_t vaddr, uint64_t size)
{
    struct ma_rmw_event ev = {.pc   = (const void *)cp->pc,
                              .func = cp->func,
                              .addr = (void *)vaddr,
                              .size = size,
                              .op   = RMW_OP_ADD,
                              .val  = {.u64 = 0}};
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_MA_RMW, &ev, NULL);
}

void
emit_ma_xchg(capture_point *cp, uint64_t vaddr, uint64_t size)
{
    struct ma_xchg_event ev = {.pc   = (const void *)cp->pc,
                               .func = cp->func,
                               .addr = (void *)vaddr,
                               .size = size,
                               .val  = {.u64 = 0}};
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_MA_XCHG, &ev, NULL);
}

void
emit_ma_cmpxchg(capture_point *cp, uint64_t vaddr, uint64_t size)
{
    struct ma_cmpxchg_event ev = {.pc   = (const void *)cp->pc,
                                  .func = cp->func,
                                  .addr = (void *)vaddr,
                                  .size = size,
                                  .cmp  = {.u64 = 0},
                                  .val  = {.u64 = 0}};
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_MA_CMPXCHG, &ev, NULL);
}

void
emit_memaccess(unsigned int cpu_index, qemu_plugin_meminfo_t info,
               uint64_t vaddr, void *udata)
{
    if (!qemu_instrumentation_enabled(cpu_index)) {
        return;
    }

    bool atomic_like = ((uintptr_t)udata) != 0;
    capture_point cp = {.chain_id = INTERCEPT_EVENT, .func = __FUNCTION__};
#if defined(QLOTTO_ENABLED)
    cp.cpu_cost = qemu_instruction_icount();
#endif
    bool is_store = qemu_plugin_mem_is_store(info);

    if (is_store && atomic_like) {
        emit_ma_awrite(&cp, vaddr, qemu_memaccess_size(info));
        return;
    }

    if (is_store) {
        emit_ma_write(&cp, vaddr, qemu_memaccess_size(info));
        return;
    }

    if (atomic_like) {
        emit_ma_aread(&cp, vaddr, qemu_memaccess_size(info));
        return;
    }

    emit_ma_read(&cp, vaddr, qemu_memaccess_size(info));
}
