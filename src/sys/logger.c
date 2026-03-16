#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#define LOGGER_IMPL
#include <lotto/sys/abort.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger.h>
#include <lotto/sys/stdio.h>

static enum logger_level _level = LOGGER_INFO;
static int _fd                  = -1;

#define LOGGER_PRINTF                                                          \
    do {                                                                       \
        va_list args;                                                          \
        va_start(args, fmt);                                                   \
        if (_fd >= 0)                                                          \
            sys_vdprintf(_fd, fmt, args);                                      \
        va_end(args);                                                          \
    } while (0)

void
logger_set_level(enum logger_level l)
{
    _level = l;
}

int
logger_fd(void)
{
    return _fd;
}

void
logger(enum logger_level l, int fd)
{
    _level = l;
    _fd    = fd;
}

__attribute__((format(printf, 1, 2))) void
logger_printf(const char *fmt, ...)
{
    LOGGER_PRINTF;
}

__attribute__((format(printf, 1, 2))) void
logger_fatalf(const char *fmt, ...)
{
    LOGGER_PRINTF;
    sys_abort();
}

__attribute__((format(printf, 1, 2))) void
logger_errorf(const char *fmt, ...)
{
    LOGGER_PRINTF;
}

__attribute__((format(printf, 1, 2))) void
logger_warnf(const char *fmt, ...)
{
    if (_level >= LOGGER_WARN)
        LOGGER_PRINTF;
}

__attribute__((format(printf, 1, 2))) void
logger_infof(const char *fmt, ...)
{
    if (_level >= LOGGER_INFO)
        LOGGER_PRINTF;
}

__attribute__((format(printf, 1, 2))) void
logger_debugf(const char *fmt, ...)
{
    if (_level >= LOGGER_DEBUG)
        LOGGER_PRINTF;
}
