#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#define LOGGER_IMPL
#include <lotto/sys/abort.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger.h>
#include <lotto/sys/stdio.h>

static enum logger_level _level = LOGGER_INFO;
static int _fd                  = -1;

static void
_logger_prefix(const char *prefix, const char *file, int line)
{
    if (_fd < 0 || prefix == NULL || file == NULL || line <= 0) {
        return;
    }
    sys_dprintf(_fd, "[%s/%s:%d] ", prefix, file, line);
}

static void
_logger_vprintf(const char *prefix, const char *file, int line, const char *fmt,
                va_list args)
{
    if (_fd < 0) {
        return;
    }
    _logger_prefix(prefix, file, line);
    sys_vdprintf(_fd, fmt, args);
}

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

__attribute__((format(printf, 4, 5))) void
_logger_fatalf(const char *prefix, const char *file, int line, const char *fmt,
               ...)
{
    va_list args;
    va_start(args, fmt);
    _logger_vprintf(prefix, file, line, fmt, args);
    va_end(args);
    sys_abort();
}

__attribute__((format(printf, 4, 5))) void
_logger_errorf(const char *prefix, const char *file, int line, const char *fmt,
               ...)
{
    va_list args;
    va_start(args, fmt);
    _logger_vprintf(prefix, file, line, fmt, args);
    va_end(args);
}

__attribute__((format(printf, 4, 5))) void
_logger_warnf(const char *prefix, const char *file, int line, const char *fmt,
              ...)
{
    if (_level >= LOGGER_WARN) {
        va_list args;
        va_start(args, fmt);
        _logger_vprintf(prefix, file, line, fmt, args);
        va_end(args);
    }
}

__attribute__((format(printf, 4, 5))) void
_logger_infof(const char *prefix, const char *file, int line, const char *fmt,
              ...)
{
    if (_level >= LOGGER_INFO) {
        va_list args;
        va_start(args, fmt);
        _logger_vprintf(prefix, file, line, fmt, args);
        va_end(args);
    }
}

__attribute__((format(printf, 4, 5))) void
_logger_debugf(const char *prefix, const char *file, int line, const char *fmt,
               ...)
{
    if (_level >= LOGGER_DEBUG) {
        va_list args;
        va_start(args, fmt);
        _logger_vprintf(prefix, file, line, fmt, args);
        va_end(args);
    }
}

__attribute__((format(printf, 1, 2))) void
logger_printf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    _logger_vprintf(NULL, NULL, 0, fmt, args);
    va_end(args);
}

__attribute__((format(printf, 1, 2))) void
logger_fatalf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    _logger_vprintf(NULL, NULL, 0, fmt, args);
    va_end(args);
    sys_abort();
}

__attribute__((format(printf, 1, 2))) void
logger_errorf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    _logger_vprintf(NULL, NULL, 0, fmt, args);
    va_end(args);
}

__attribute__((format(printf, 1, 2))) void
logger_warnf(const char *fmt, ...)
{
    if (_level >= LOGGER_WARN) {
        va_list args;
        va_start(args, fmt);
        _logger_vprintf(NULL, NULL, 0, fmt, args);
        va_end(args);
    }
}

__attribute__((format(printf, 1, 2))) void
logger_infof(const char *fmt, ...)
{
    if (_level >= LOGGER_INFO) {
        va_list args;
        va_start(args, fmt);
        _logger_vprintf(NULL, NULL, 0, fmt, args);
        va_end(args);
    }
}

__attribute__((format(printf, 1, 2))) void
logger_debugf(const char *fmt, ...)
{
    if (_level >= LOGGER_DEBUG) {
        va_list args;
        va_start(args, fmt);
        _logger_vprintf(NULL, NULL, 0, fmt, args);
        va_end(args);
    }
}
