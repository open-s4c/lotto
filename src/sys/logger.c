/*
 */
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#define LOG_IMPL
#include <lotto/sys/abort.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger.h>
#include <lotto/sys/stdio.h>

static enum log_level _level = LOG_INFO;
static FILE *_fp             = NULL;

#define LOG_PRINTF                                                             \
    do {                                                                       \
        va_list args;                                                          \
        va_start(args, fmt);                                                   \
        if (_fp)                                                               \
            sys_vfprintf(_fp, fmt, args);                                      \
        else                                                                   \
            sys_vdprintf(STDOUT_FILENO, fmt, args);                            \
        va_end(args);                                                          \
    } while (0)

void
logger_set_level(enum log_level l)
{
    _level = l;
}

FILE *
logger_fp(void)
{
    return _fp;
}

void
logger(enum log_level l, FILE *fp)
{
    _level = l;
    _fp    = fp;
}

__attribute__((format(printf, 1, 2))) void
log_printf(const char *fmt, ...)
{
    LOG_PRINTF;
}

__attribute__((format(printf, 1, 2))) void
log_fatalf(const char *fmt, ...)
{
    LOG_PRINTF;
    fflush(_fp);
    sys_abort();
}

__attribute__((format(printf, 1, 2))) void
log_errorf(const char *fmt, ...)
{
    LOG_PRINTF;
    fflush(_fp);
}

__attribute__((format(printf, 1, 2))) void
log_warnf(const char *fmt, ...)
{
    if (_level >= LOG_WARN)
        LOG_PRINTF;
}

__attribute__((format(printf, 1, 2))) void
log_infof(const char *fmt, ...)
{
    if (_level >= LOG_INFO)
        LOG_PRINTF;
}

__attribute__((format(printf, 1, 2))) void
log_debugf(const char *fmt, ...)
{
    if (_level >= LOG_DEBUG)
        LOG_PRINTF;
}
