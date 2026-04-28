#include <lotto/driver/flagmgr.h>
#include <lotto/sys/assert.h>

NEW_PUBLIC_CALLBACK_FLAG(MEMMGR_RUNTIME, "", "memmgr-runtime", "MEMMGR",
                         "runtime memory manager chain "
                         "leakcheck|mempool|uaf[:leakcheck|mempool|uaf]*",
                         flag_sval(""), {})
NEW_PUBLIC_CALLBACK_FLAG(MEMMGR_USER, "", "memmgr-user", "MEMMGR",
                         "user memory manager chain "
                         "leakcheck|mempool|uaf[:leakcheck|mempool|uaf]*",
                         flag_sval(""), {})
FLAG_GETTER(memmgr_runtime, MEMMGR_RUNTIME)
FLAG_GETTER(memmgr_user, MEMMGR_USER)
