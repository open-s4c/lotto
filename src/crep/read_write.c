/*
 */
#include <crep.h>
#include <stddef.h>
#include <time.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/uio.h>
#define LOG_PREFIX LOG_CUR_FILE
#define LOG_BLOCK  LOG_CUR_BLOCK
#include <lotto/sys/logger_block.h>

#ifndef QLOTTO_ENABLED
CREP_FUNC(SIG(ssize_t, read, int, fd, void *, buf, size_t, count),
          MAP(.retsize = sizeof(ssize_t),
              .params  = {{.size = count, .data = buf}, NULL}))

CREP_FUNC(SIG(ssize_t, write, int, fd, const void *, buf, size_t, count),
          MAP(.retsize = sizeof(ssize_t)))

CREP_FUNC(SIG(ssize_t, writev, int, fd, const struct iovec *, iov, int, iovcnt),
          MAP(.retsize = sizeof(ssize_t)))

CREP_FUNC(SIG(off_t, lseek, int, fd, off_t, offset, int, whence),
          MAP(.retsize = sizeof(off_t)))

CREP_FUNC(SIG(ssize_t, pread, int, fd, void *, buf, size_t, count, off_t,
              offset),
          MAP(.retsize = sizeof(ssize_t),
              .params  = {{.size = count, .data = buf}, NULL}))

CREP_FUNC(SIG(ssize_t, pwrite, int, fd, const void *, buf, size_t, count, off_t,
              offset),
          MAP(.retsize = sizeof(ssize_t)))
#endif
