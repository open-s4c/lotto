/**
 * @file event.h
 * @brief QLotto frontend declarations for event.
 */
#ifndef LOTTO_QEMU_EVENT_H
#define LOTTO_QEMU_EVENT_H

#include <stdint.h>

#include <dice/events/memaccess.h>
#include <dice/events/stacktrace.h>
#include <lotto/base/category.h>
#include <lotto/base/context.h>
#include <lotto/base/map.h>
#include <lotto/modules/deadlock/events.h>
#include <lotto/runtime/capture_point.h>
#include <lotto/runtime/context_payload.h>
#include <lotto/runtime/ingress_events.h>
#include <lotto/runtime/module_events.h>

#define EVENT_QLOTTO_EXIT 196

typedef struct qlotto_exit_event {
    uint32_t reason;
} qlotto_exit_event_t;

typedef struct event_s {
    mapitem_t ti;
    type_id type;
    char *func_name;
} eventi_t;

typedef struct event_context_s {
    context_t *ctx;
    eventi_t *event;
} event_context_t;

void qlotto_add_event(map_t *emap, uint64_t e_pc, type_id type,
                      char *func_name);
void qlotto_del_event(map_t *emap, uint64_t e_pc);
eventi_t *qlotto_get_event(map_t *emap, uint64_t e_pc);

static inline void
qlotto_context_set_event(context_t *ctx, type_id type, type_id src_type,
                         context_phase_t phase)
{
    ctx->type     = type;
    ctx->src_type = src_type;
    ctx->cp       = NULL;
    ctx->phase    = phase;
    ctx->cat      = context_event_category(type != 0 ? type : src_type);
}

static inline void
qlotto_context_set_semantics(context_t *ctx, category_t cat)
{
    ctx->cat      = cat;
    ctx->type     = 0;
    ctx->src_type = 0;
    ctx->cp       = NULL;
    ctx->phase    = CONTEXT_PHASE_EVENT;

    switch (cat) {
        case CAT_BEFORE_READ:
            ctx->type     = EVENT_BEFORE_READ;
            ctx->src_type = EVENT_MA_READ;
            ctx->phase    = CONTEXT_PHASE_BEFORE;
            break;
        case CAT_BEFORE_WRITE:
            ctx->type     = EVENT_BEFORE_WRITE;
            ctx->src_type = EVENT_MA_WRITE;
            ctx->phase    = CONTEXT_PHASE_BEFORE;
            break;
        case CAT_BEFORE_AREAD:
            ctx->type     = EVENT_BEFORE_AREAD;
            ctx->src_type = EVENT_MA_AREAD;
            ctx->phase    = CONTEXT_PHASE_BEFORE;
            break;
        case CAT_BEFORE_AWRITE:
            ctx->type     = EVENT_BEFORE_AWRITE;
            ctx->src_type = EVENT_MA_AWRITE;
            ctx->phase    = CONTEXT_PHASE_BEFORE;
            break;
        case CAT_BEFORE_RMW:
            ctx->type     = EVENT_BEFORE_RMW;
            ctx->src_type = EVENT_MA_RMW;
            ctx->phase    = CONTEXT_PHASE_BEFORE;
            break;
        case CAT_BEFORE_XCHG:
            ctx->type     = EVENT_BEFORE_XCHG;
            ctx->src_type = EVENT_MA_XCHG;
            ctx->phase    = CONTEXT_PHASE_BEFORE;
            break;
        case CAT_BEFORE_CMPXCHG:
            ctx->type     = EVENT_BEFORE_CMPXCHG;
            ctx->src_type = EVENT_MA_CMPXCHG;
            ctx->phase    = CONTEXT_PHASE_BEFORE;
            break;
        case CAT_BEFORE_FENCE:
            ctx->type     = EVENT_BEFORE_FENCE;
            ctx->src_type = EVENT_MA_FENCE;
            ctx->phase    = CONTEXT_PHASE_BEFORE;
            break;
        case CAT_FUNC_ENTRY:
            ctx->type     = EVENT_FUNC_ENTRY;
            ctx->src_type = EVENT_STACKTRACE_ENTER;
            ctx->phase    = CONTEXT_PHASE_BEFORE;
            break;
        case CAT_FUNC_EXIT:
            ctx->type     = EVENT_FUNC_EXIT;
            ctx->src_type = EVENT_STACKTRACE_EXIT;
            ctx->phase    = CONTEXT_PHASE_AFTER;
            break;
        case CAT_RSRC_ACQUIRING:
            ctx->type     = EVENT_RSRC_ACQUIRING;
            ctx->src_type = EVENT_RSRC_ACQUIRING;
            ctx->phase    = CONTEXT_PHASE_BEFORE;
            break;
        case CAT_RSRC_RELEASED:
            ctx->type     = EVENT_RSRC_RELEASED;
            ctx->src_type = EVENT_RSRC_RELEASED;
            ctx->phase    = CONTEXT_PHASE_AFTER;
            break;
        case CAT_SYS_YIELD:
            qlotto_context_set_event(ctx, EVENT_SYS_YIELD, EVENT_SYS_YIELD,
                                     CONTEXT_PHASE_EVENT);
            break;
        case CAT_REGION_IN:
            qlotto_context_set_event(ctx, EVENT_REGION_IN, EVENT_REGION_IN,
                                     CONTEXT_PHASE_EVENT);
            break;
        case CAT_REGION_OUT:
            qlotto_context_set_event(ctx, EVENT_REGION_OUT, EVENT_REGION_OUT,
                                     CONTEXT_PHASE_EVENT);
            break;
        default:
            break;
    }
}


#endif // LOTTO_QEMU_EVENT_H
