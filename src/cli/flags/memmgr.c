/*
 */
#define LOG_PREFIX LOG_CUR_FILE
#define LOG_BLOCK  LOG_CUR_BLOCK
#include <lotto/cli/flagmgr.h>
#include <lotto/sys/assert.h>

NEW_PUBLIC_CALLBACK_FLAG(MEMMGR_RUNTIME, "", "memmgr-runtime",
                         "leakcheck|mempool|uaf[:leakcheck|mempool|uaf]*",
                         "runtime memory manager chain", flag_sval("mempool"),
                         {})
NEW_PUBLIC_CALLBACK_FLAG(MEMMGR_USER, "", "memmgr-user",
                         "leakcheck|mempool|uaf[:leakcheck|mempool|uaf]*",
                         "user memory manager chain", flag_sval(""), {})
FLAG_GETTER(memmgr_runtime, MEMMGR_RUNTIME)
FLAG_GETTER(memmgr_user, MEMMGR_USER)
