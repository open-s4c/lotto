#include <stdbool.h>
#include <stdint.h>

#include <lotto/base/context.h>
#include <lotto/base/marshable.h>
#include <lotto/base/reason.h>
#include <lotto/base/tidset.h>
#include <lotto/sys/stream.h>

tidset_t *
replay_get_schedulable_tasks()
{
    return NULL;
}

uint64_t
pct_get_kmax(const marshable_t *m)
{
    return 1;
}

void
ichpt_load(stream_t *s, bool reset)
{
}

void (*init_lock_pairs_cb)(void) = NULL;

void
init_lock_pair(const char *acq_name, const char *rel_name)
{
}

__attribute__((noreturn)) void (*lotto_exit_locks)(const context_t *ctx,
                                                   reason_t reason) = NULL;
