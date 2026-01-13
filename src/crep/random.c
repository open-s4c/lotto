/*
 */
#include <crep.h>
#include <stddef.h>
#include <time.h>

#define LOGGER_PREFIX LOGGER_CUR_FILE
#define LOGGER_BLOCK  LOGGER_CUR_BLOCK
#include <lotto/sys/logger_block.h>
#include <sys/random.h>

CREP_FUNC(SIG(ssize_t, getrandom, void *, buf, size_t, buflen, unsigned int,
              flags),
          MAP(.retsize = sizeof(ssize_t),
              .params  = {{.size = buflen, .data = buf}, NULL}))
