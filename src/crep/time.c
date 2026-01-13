/*
 */
#include <crep.h>
#include <stddef.h>
#include <time.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/types.h>

#define LOGGER_PREFIX LOGGER_CUR_FILE
#define LOGGER_BLOCK  LOGGER_CUR_BLOCK
#include <lotto/sys/assert.h>
#include <lotto/sys/logger_block.h>

#if 0
CREP_FUNC(SIG(time_t, time, time_t *, tloc),
          MAP(.retsize = sizeof(time_t),
              .params  = {{.size = sizeof(time_t), .data = tloc}, NULL}))
#endif

/* @note The manual page defines tz as `struct timezone *`, however the header
 * file sys/time.h defines it `void*` but also mentions in a comment that it
 * should contain sizeof struct timezone. */
#if 0
CREP_FUNC(SIG(int, gettimeofday, struct timeval *, tv, void *, tz),
          MAP(.retsize = sizeof(int),
              .params  = {
                   {.size = sizeof(struct timeval), .data = tv},
                   {.size = sizeof(unsigned long), .data = tz},
                   NULL,
              }))
#endif

#if 0
CREP_FUNC(SIG(int, clock_gettime, clockid_t, clockid, struct timespec *, tp),
          MAP(.retsize = sizeof(int),
              .params  = {
                   {.size = sizeof(struct timespec), .data = tp},
                   NULL,
              }))
#endif
