/*
 */
#include <stdlib.h>

#define LOGGER_PREFIX LOGGER_CUR_FILE
#define LOGGER_BLOCK  LOGGER_CUR_BLOCK

#include <mempool.h>

#include <lotto/brokers/statemgr.h>
#include <lotto/cli/flagmgr.h>
#include <lotto/sys/logger_block.h>
#include <lotto/sys/stdio.h>
#include <lotto/util/macros.h>

#if LOTTO_MEMMGR_USER
    #define FLAG_OPT   "lotto_mempool-user-size"
    #define FLAG_DESCR "user lotto_mempool size"
#elif LOTTO_MEMMGR_RUNTIME
    #define FLAG_OPT   "lotto_mempool-runtime-size"
    #define FLAG_DESCR "runtime lotto_mempool size"
#endif

#define LOTTO_MEMPOOL_DEFAULT_SIZE ((size_t)128 * 1024 * 1024)
#define MAX_LEN                    1024

typedef struct lotto_mempool_size_s {
    marshable_t m;
    size_t size;
} lotto_mempool_size_t;

static lotto_mempool_size_t _lotto_mempool_size;

REGISTER_PRINTABLE_STATE(START, _lotto_mempool_size, {
    logger_infof(FLAG_OPT " = %lu\n", _lotto_mempool_size.size);
})

NEW_CALLBACK_FLAG(LOTTO_MEMPOOL_SIZE, "", FLAG_OPT, "INT", FLAG_DESCR,
                  flag_uval(LOTTO_MEMPOOL_DEFAULT_SIZE), {
                      char str[MAX_LEN];
                      sys_sprintf(str, "%lu",
                                  _lotto_mempool_size.size == 0 ?
                                      _lotto_mempool_size.size = as_uval(v) :
                                      _lotto_mempool_size.size);
                      setenv(XSTR(LOTTO_MEMPOOL_SIZE), str, true);
                  })
