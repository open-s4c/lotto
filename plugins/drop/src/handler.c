/*
 */
#include "lotto/base/reason.h"
#include "lotto/base/slot.h"
#define LOG_PREFIX LOG_CUR_FILE
#define LOG_BLOCK  LOG_CUR_BLOCK
#include <category.h>
#include <state.h>

#include <lotto/base/tidmap.h>
#include <lotto/brokers/statemgr.h>
#include <lotto/engine/dispatcher.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger_block.h>
#include <lotto/util/macros.h>
#include <lotto/util/once.h>

static category_t CAT_DROP;

/*******************************************************************************
 * handler
 ******************************************************************************/
STATIC void
_drop_handle(const context_t *ctx, event_t *e)
{
    once(CAT_DROP = drop_category());
    ASSERT(e);
    if (e->skip || !drop_config()->enabled)
        return;

    ASSERT(ctx);
    if (ctx->cat == CAT_DROP)
        e->reason = REASON_IGNORE;
}
REGISTER_HANDLER(SLOT_DROP, _drop_handle)
