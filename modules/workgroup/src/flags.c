#include "state.h"
#include <lotto/driver/flagmgr.h>

NEW_PUBLIC_PRETTY_CALLBACK_FLAG(
    WORKGROUP_THREAD_START_POLICY, "", "workgroup-thread-start-policy",
    "threads that auto-enter a workgroup from program start", flag_uval(0),
    STR_CONVERTER_GET(workgroup_thread_start_policy_str,
                      workgroup_thread_start_policy_from,
                      sizeof(workgroup_thread_start_policy_t),
                      workgroup_thread_start_policy_all_str),
    {
        workgroup_config()->thread_start_policy =
            (workgroup_thread_start_policy_t)as_uval(v);
    })
