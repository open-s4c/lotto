#ifndef LOTTO_LOGGER_H
#define LOTTO_LOGGER_H

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <dice/log.h>

#ifndef LOG_CUR_FILE
    #define LOG_CUR_FILE                                                       \
        (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

enum log_level {
    LOG_ERROR = LOG_LEVEL_FATAL,
    LOG_WARN  = LOG_LEVEL_INFO,
    LOG_INFO  = LOG_LEVEL_INFO,
    LOG_DEBUG = LOG_LEVEL_DEBUG,
};

static inline void
logger_set_level(enum log_level level)
{
    (void)level;
}

static inline FILE *
logger_fp(void)
{
    return stdout;
}

static inline void
logger(enum log_level level, FILE *fp)
{
    (void)level;
    (void)fp;
}

typedef enum log_emit_level {
    LOG_EMIT_FATAL,
    LOG_EMIT_ERROR,
    LOG_EMIT_WARN,
    LOG_EMIT_INFO,
    LOG_EMIT_DEBUG,
} log_emit_level_t;


// dice has no log_error yet
#define log_error log_warn

static inline void
log_emit(log_emit_level_t level, const char *fmt, va_list args)
{
    char buf[LOG_MAX_LEN];
    int n = vsnprintf(buf, sizeof(buf), fmt, args);
    if (n < 0)
        return;

    switch (level) {
        case LOG_EMIT_FATAL:
            log_fatal(": %s", buf);
            break;
        case LOG_EMIT_ERROR:
            log_printf(": %s", buf);
            break;
        case LOG_EMIT_WARN:
            log_printf(": %s", buf);
            break;
        case LOG_EMIT_INFO:
            log_printf(": %s", buf);
            break;
        case LOG_EMIT_DEBUG:
            log_printf(": %s", buf);
            break;
    }
}

#if defined(__clang__) || defined(__GNUC__)
    #define LOTTO_PRINTF_ATTR(pos)                                             \
        __attribute__((format(printf, pos, (pos + 1))))
#else
    #define LOTTO_PRINTF_ATTR(pos)
#endif

#define DEFINE_LOG_WRAPPER(fn, lvl)                                            \
    static inline void fn(const char *fmt, ...) LOTTO_PRINTF_ATTR(1);          \
    static inline void fn(const char *fmt, ...)                                \
    {                                                                          \
        va_list args;                                                          \
        va_start(args, fmt);                                                   \
        log_emit((lvl), fmt, args);                                            \
        va_end(args);                                                          \
    }

DEFINE_LOG_WRAPPER(log_fatalf, LOG_EMIT_FATAL)
DEFINE_LOG_WRAPPER(log_errorf, LOG_EMIT_ERROR)
DEFINE_LOG_WRAPPER(log_warnf, LOG_EMIT_WARN)
DEFINE_LOG_WRAPPER(log_infof, LOG_EMIT_INFO)
DEFINE_LOG_WRAPPER(log_debugf, LOG_EMIT_DEBUG)

#undef DEFINE_LOG_WRAPPER
#undef LOTTO_PRINTF_ATTR

#ifndef LOG_IMPL
    #define log_debugln(fmt, ...) log_debugf(fmt "\n", ##__VA_ARGS__)
    #define log_infoln(fmt, ...)  log_infof(fmt "\n", ##__VA_ARGS__)
    #define log_println(fmt, ...) log_printf(fmt "\n", ##__VA_ARGS__)

    #ifdef LOG_DISABLE
        #error
        #include <lotto/util/unused.h>
        #undef log_infof
        #define log_infof(...) LOTTO_UNUSED(__VA_ARGS__)
        #undef log_debugf
        #define log_debugf(...) LOTTO_UNUSED(__VA_ARGS__)
        #undef log_printf
        #define log_printf(...) LOTTO_UNUSED(__VA_ARGS__)
    #endif
#endif

#endif /* LOTTO_LOGGER_H */
