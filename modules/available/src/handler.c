#define LOGGER_PREFIX LOGGER_CUR_FILE
#define LOGGER_BLOCK  LOGGER_CUR_BLOCK
#include <lotto/engine/dispatcher.h>
#include <lotto/modules/available/state.h>
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
