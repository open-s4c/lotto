/*
 */
#ifndef LOTTO_LOGGER_H
#define LOTTO_LOGGER_H

#include <stdio.h>
#include <string.h> // strrchr

#if !defined(LOGGER_IMPL) && defined(LOGGER_DISABLE)
    #ifdef LOGGER_PREFIX
        #undef LOGGER_PREFIX
        #define LOGGER_PREFIX ""
    #endif
#endif

enum logger_level {
    LOGGER_ERROR,
    LOGGER_WARN,
    LOGGER_INFO,
    LOGGER_DEBUG,
};

void logger_set_level(enum logger_level l);
FILE *logger_fp(void);
void logger(enum logger_level l, FILE *fp);

#define PRINTFLIKE __attribute__((format(printf, 1, 2)))

void logger_fatalf(const char *fmt, ...) PRINTFLIKE;
void logger_errorf(const char *fmt, ...) PRINTFLIKE;
void logger_warnf(const char *fmt, ...) PRINTFLIKE;
void logger_infof(const char *fmt, ...) PRINTFLIKE;
void logger_debugf(const char *fmt, ...) PRINTFLIKE;
void logger_printf(const char *fmt, ...) PRINTFLIKE;

#undef PRINTFLIKE

#ifndef LOGGER_IMPL
    #define logger_debugln(fmt, ...) logger_debugf(fmt "\n", ##__VA_ARGS__)
    #define logger_infoln(fmt, ...)  logger_infof(fmt "\n", ##__VA_ARGS__)
    #define logger_println(fmt, ...) logger_printf(fmt "\n", ##__VA_ARGS__)

    #define LOGGER_CUR_FILE                                                    \
        (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

    #ifdef LOGGER_PREFIX
        #define logger_fatalf(fmt, ...)                                        \
            logger_fatalf("[%21s] " fmt, LOGGER_PREFIX, ##__VA_ARGS__)

        #define logger_errorf(fmt, ...)                                        \
            logger_errorf("[%21s] " fmt, LOGGER_PREFIX, ##__VA_ARGS__)

        #define logger_warnf(fmt, ...)                                         \
            logger_warnf("[%21s] " fmt, LOGGER_PREFIX, ##__VA_ARGS__)

        #define logger_infof(fmt, ...)                                         \
            logger_infof("[%21s] " fmt, LOGGER_PREFIX, ##__VA_ARGS__)

        #define logger_debugf(fmt, ...)                                        \
            logger_debugf("[%21s] " fmt, LOGGER_PREFIX, ##__VA_ARGS__)

    #endif /* LOGGER_PREFIX */

    #ifdef LOGGER_DISABLE
        #include <lotto/util/unused.h>

        #undef logger_infof
        #define logger_infof(...) LOTTO_UNUSED(__VA_ARGS__)
        #undef logger_debugf
        #define logger_debugf(...) LOTTO_UNUSED(__VA_ARGS__)
        #undef logger_printf
        #define logger_printf(...) LOTTO_UNUSED(__VA_ARGS__)
    #endif
#endif

#endif /* LOTTO_LOGGER_H */
