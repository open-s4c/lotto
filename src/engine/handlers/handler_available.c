/*
 */
#define LOG_PREFIX LOG_CUR_FILE
#define LOG_BLOCK  LOG_CUR_BLOCK
#include <lotto/engine/dispatcher.h>
#include <lotto/states/handlers/available.h>
#include <lotto/sys/logger_block.h>
#include <lotto/util/macros.h>

STATIC void
_available_handle(const context_t *ctx, event_t *e)
{
    if (!available_config()->enabled) {
        return;
    }
    ASSERT(e);
    tidset_copy(get_available_tasks(), &e->tset);
}
REGISTER_HANDLER(SLOT_AVAILABLE, _available_handle);
