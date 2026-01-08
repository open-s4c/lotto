#ifndef LOTTO_LOGGER_H
#define LOTTO_LOGGER_H

#include <stdio.h>
#include <string.h> // strrchr

#if !defined(LOG_IMPL) && defined(LOG_DISABLE)
    #ifdef LOG_PREFIX
        #undef LOG_PREFIX
        #define LOG_PREFIX ""
    #endif
#endif

enum log_level {
    LOG_ERROR,
    LOG_WARN,
    LOG_INFO,
    LOG_DEBUG,
};

void logger_set_level(enum log_level l);
FILE *logger_fp(void);
void logger(enum log_level l, FILE *fp);

#define PRINTFLIKE __attribute__((format(printf, 1, 2)))

void log_fatalf(const char *fmt, ...) PRINTFLIKE;
void log_errorf(const char *fmt, ...) PRINTFLIKE;
void log_warnf(const char *fmt, ...) PRINTFLIKE;
void log_infof(const char *fmt, ...) PRINTFLIKE;
void log_debugf(const char *fmt, ...) PRINTFLIKE;
void log_printf(const char *fmt, ...) PRINTFLIKE;

#undef PRINTFLIKE

#ifndef LOG_IMPL
    #define log_debugln(fmt, ...) log_debugf(fmt "\n", ##__VA_ARGS__)
    #define log_infoln(fmt, ...)  log_infof(fmt "\n", ##__VA_ARGS__)
    #define log_println(fmt, ...) log_printf(fmt "\n", ##__VA_ARGS__)

    #define log_debug(fmt, ...) log_debugf(fmt "\n", ##__VA_ARGS__)
    #define log_info(fmt, ...)  log_infof(fmt "\n", ##__VA_ARGS__)
    #define log_print(fmt, ...) log_printf(fmt "\n", ##__VA_ARGS__)
    #define log_fatal(fmt, ...) log_fatalf(fmt "\n", ##__VA_ARGS__)

    #define LOG_CUR_FILE                                                       \
        (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

    #ifdef LOG_PREFIX
        #define log_fatalf(fmt, ...)                                           \
            log_fatalf("[%21s] " fmt, LOG_PREFIX, ##__VA_ARGS__)

        #define log_errorf(fmt, ...)                                           \
            log_errorf("[%21s] " fmt, LOG_PREFIX, ##__VA_ARGS__)

        #define log_warnf(fmt, ...)                                            \
            log_warnf("[%21s] " fmt, LOG_PREFIX, ##__VA_ARGS__)

        #define log_infof(fmt, ...)                                            \
            log_infof("[%21s] " fmt, LOG_PREFIX, ##__VA_ARGS__)

        #define log_debugf(fmt, ...)                                           \
            log_debugf("[%21s] " fmt, LOG_PREFIX, ##__VA_ARGS__)

    #endif /* LOG_PREFIX */

    #ifdef LOG_DISABLE
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
