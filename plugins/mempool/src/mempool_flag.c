/*
 */
#include <stdlib.h>

#define LOG_PREFIX LOG_CUR_FILE
#define LOG_BLOCK  LOG_CUR_BLOCK

#include <mempool.h>

#include <lotto/brokers/statemgr.h>
#include <lotto/cli/flagmgr.h>
#include <lotto/sys/logger_block.h>
#include <lotto/sys/stdio.h>
#include <lotto/util/macros.h>

#if LOTTO_MEMMGR_USER
    #define FLAG_OPT   "mempool-user-size"
    #define FLAG_DESCR "user mempool size"
#elif LOTTO_MEMMGR_RUNTIME
    #define FLAG_OPT   "mempool-runtime-size"
    #define FLAG_DESCR "runtime mempool size"
#endif

#define LOTTO_MEMPOOL_DEFAULT_SIZE ((size_t)128 * 1024 * 1024)
#define MAX_LEN                    1024

typedef struct mempool_size_s {
    marshable_t m;
    size_t size;
} mempool_size_t;

static mempool_size_t _mempool_size;

REGISTER_PRINTABLE_STATE(START, _mempool_size, {
    log_infof(FLAG_OPT " = %lu\n", _mempool_size.size);
})

NEW_CALLBACK_FLAG(MEMPOOL_SIZE, "", FLAG_OPT, "INT", FLAG_DESCR,
                  flag_uval(LOTTO_MEMPOOL_DEFAULT_SIZE), {
                      char str[MAX_LEN];
                      sys_sprintf(str, "%lu",
                                  _mempool_size.size == 0 ?
                                      _mempool_size.size = as_uval(v) :
                                      _mempool_size.size);
                      setenv(XSTR(MEMPOOL_SIZE), str, true);
                  })
