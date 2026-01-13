/*
 */
/**
 * - When a thead is created it gets priority 0
 * - __lotto_priority(false, prio) is ignored
 * - __lotto_priority(true, prio) is intercepted and the task gets his
 * priority updated to prio
 * - If the event is immutable, the handler doesn't filter out anything
 * - If the event is mutable, the handler filters out all tasks whose priority
 * is not maximum
 */
#define LOGGER_PREFIX LOGGER_CUR_FILE
#define LOGGER_BLOCK  LOGGER_CUR_BLOCK
#include <state.h>

#include <lotto/base/tidmap.h>
#include <lotto/brokers/statemgr.h>
#include <lotto/engine/dispatcher.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger_block.h>
#include <lotto/util/macros.h>
#include <lotto/util/once.h>

typedef struct {
    marshable_t m;
    uint64_t anytask_count;
} state_t;

static state_t _state;

STATIC void
_anystall_handle(const context_t *ctx, event_t *e)
{
    ASSERT(e);
    if (e->skip || !anystall_config()->enabled)
        return;

    ASSERT(ctx);
    ASSERT(ctx->id != NO_TASK);

    if (tidset_size(&e->tset) != 0) {
        _state.anytask_count = 0;
        return;
    }

    _state.anytask_count++;

    if (_state.anytask_count > anystall_config()->anytask_limit) {
        logger_fatalf(
            "Scheduled ANY_TASK %lu times in a row. Possible deadlock?\n",
            _state.anytask_count);
    }
}
REGISTER_HANDLER(SLOT_ANYSTALL, _anystall_handle)
