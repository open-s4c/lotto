#define LOGGER_BLOCK LOGGER_CUR_BLOCK
#include <lotto/base/tidbag.h>
#include <lotto/brokers/statemgr.h>
#include <lotto/engine/dispatcher.h>
#include <lotto/modules/region_filter/state.h>
#include <lotto/sys/logger_block.h>
#include <lotto/util/macros.h>

static tidbag_t _in_region;

REGISTER_EPHEMERAL(_in_region, { tidbag_init(&_in_region); })

STATIC void
_region_filter_handle(const context_t *ctx, event_t *e)
{
    ASSERT(ctx && ctx->id != NO_TASK);
    ASSERT(e);

    if (!region_filter_config()->enabled)
        return;

    task_id tid = ctx->vid != NO_TASK ? ctx->vid : ctx->id;

    switch (ctx->cat) {
        case CAT_REGION_IN:
            if (!tidbag_has(&_in_region, tid)) {
                logger_infof("enter region %lx\n", tid);
            }
            tidbag_insert(&_in_region, tid);
            break;
        case CAT_REGION_OUT:
            tidbag_remove(&_in_region, tid);
            if (!tidbag_has(&_in_region, tid)) {
                logger_infof("leave region %lx\n", tid);
            }
            break;
        default:
            break;
    }

    if (tidbag_has(&_in_region, tid)) {
        e->filter_less = true;
    }
}
REGISTER_HANDLER(SLOT_REGION_FILTER, _region_filter_handle)
