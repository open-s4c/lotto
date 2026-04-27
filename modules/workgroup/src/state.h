#ifndef LOTTO_MODULES_WORKGROUP_STATE_H
#define LOTTO_MODULES_WORKGROUP_STATE_H

#include <stdbool.h>

#include <dice/types.h>
#include <lotto/base/task_id.h>
#include <lotto/base/tidset.h>
#include <lotto/base/marshable.h>
#include <lotto/engine/sequencer.h>
#include <lotto/runtime/capture_point.h>
#include <lotto/workgroup.h>

typedef struct workgroup_config {
    marshable_t m;
    workgroup_thread_start_policy_t thread_start_policy;
} workgroup_config_t;

workgroup_config_t *workgroup_config(void);
const char *workgroup_thread_start_policy_str(uint64_t bits);
uint64_t workgroup_thread_start_policy_from(const char *src);
void workgroup_thread_start_policy_all_str(char *dst);

bool workgroup_should_filter(metadata_t *md, chain_id chain, type_id type);
void workgroup_on_task_init(metadata_t *md);
void workgroup_on_task_fini(metadata_t *md);
void workgroup_clear_current(metadata_t *md);
void workgroup_handle_capture(const capture_point *cp, event_t *e);
void workgroup_handle_resume(const capture_point *cp);

#endif
