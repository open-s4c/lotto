/**
 * @file logger.h
 * @brief System wrapper declarations for logger.
 */
#ifndef LOTTO_LOGGER_H
#define LOTTO_LOGGER_H

#include <stdio.h>
#include <string.h> // strrchr

enum logger_level {
    LOGGER_ERROR,
    LOGGER_WARN,
    LOGGER_INFO,
    LOGGER_DEBUG,
};

void logger_set_level(enum logger_level l);
int logger_fd(void);
void logger(enum logger_level l, int fd);

#define PRINTFLIKE(FMT, ARGS) __attribute__((format(printf, FMT, ARGS)))

void _logger_fatalf(const char *prefix, const char *file, int line,
                    const char *fmt, ...) PRINTFLIKE(4, 5);
void _logger_errorf(const char *prefix, const char *file, int line,
                    const char *fmt, ...) PRINTFLIKE(4, 5);
void _logger_warnf(const char *prefix, const char *file, int line,
                   const char *fmt, ...) PRINTFLIKE(4, 5);
void _logger_infof(const char *prefix, const char *file, int line,
                   const char *fmt, ...) PRINTFLIKE(4, 5);
void _logger_debugf(const char *prefix, const char *file, int line,
                    const char *fmt, ...) PRINTFLIKE(4, 5);

void logger_fatalf(const char *fmt, ...) PRINTFLIKE(1, 2);
void logger_errorf(const char *fmt, ...) PRINTFLIKE(1, 2);
void logger_warnf(const char *fmt, ...) PRINTFLIKE(1, 2);
void logger_infof(const char *fmt, ...) PRINTFLIKE(1, 2);
void logger_debugf(const char *fmt, ...) PRINTFLIKE(1, 2);
void logger_printf(const char *fmt, ...) PRINTFLIKE(1, 2);

#undef PRINTFLIKE

static inline const char *
logger_basename(const char *file)
{
    const char *base = strrchr(file, '/');
    return base ? base + 1 : file;
}

#ifndef LOGGER_IMPL
    #define logger_debugln(fmt, ...) logger_debugf(fmt "\n", ##__VA_ARGS__)
    #define logger_infoln(fmt, ...)  logger_infof(fmt "\n", ##__VA_ARGS__)
    #define logger_println(fmt, ...) logger_printf(fmt "\n", ##__VA_ARGS__)

    #define LOGGER_CUR_FILE logger_basename(__FILE__)
    #ifdef LOGGER_PREFIX
        #define logger_fatalf(fmt, ...)                                        \
            _logger_fatalf(LOGGER_PREFIX, LOGGER_CUR_FILE, __LINE__, fmt,      \
                           ##__VA_ARGS__)

        #define logger_errorf(fmt, ...)                                        \
            _logger_errorf(LOGGER_PREFIX, LOGGER_CUR_FILE, __LINE__, fmt,      \
                           ##__VA_ARGS__)

        #define logger_warnf(fmt, ...)                                         \
            _logger_warnf(LOGGER_PREFIX, LOGGER_CUR_FILE, __LINE__, fmt,       \
                          ##__VA_ARGS__)

        #define logger_infof(fmt, ...)                                         \
            _logger_infof(LOGGER_PREFIX, LOGGER_CUR_FILE, __LINE__, fmt,       \
                          ##__VA_ARGS__)

        #define logger_debugf(fmt, ...)                                        \
            _logger_debugf(LOGGER_PREFIX, LOGGER_CUR_FILE, __LINE__, fmt,      \
                           ##__VA_ARGS__)
    #else
        #define logger_fatalf(fmt, ...)                                        \
            _logger_fatalf(NULL, NULL, 0, fmt, ##__VA_ARGS__)
        #define logger_errorf(fmt, ...)                                        \
            _logger_errorf(NULL, NULL, 0, fmt, ##__VA_ARGS__)
        #define logger_warnf(fmt, ...)                                         \
            _logger_warnf(NULL, NULL, 0, fmt, ##__VA_ARGS__)
        #define logger_infof(fmt, ...)                                         \
            _logger_infof(NULL, NULL, 0, fmt, ##__VA_ARGS__)
        #define logger_debugf(fmt, ...)                                        \
            _logger_debugf(NULL, NULL, 0, fmt, ##__VA_ARGS__)
    #endif
    #ifdef LOGGER_DISABLE
        #include <lotto/util/unused.h>

        #undef logger_infof
        #define logger_infof(...) LOTTO_UNUSED(__VA_ARGS__)
        #undef logger_debugf
        #define logger_debugf(...) LOTTO_UNUSED(__VA_ARGS__)
    #endif
#endif

#endif /* LOTTO_LOGGER_H */
