#ifndef LOTTO_LOGGER_BLOCK_H
#define LOTTO_LOGGER_BLOCK_H

#include <stdbool.h>

#include <lotto/sys/logger.h>

void logger_block_init(char *cats);
bool logger_blocked(const char *file_name);
void logger_block_fini(void);

#define LOGGER_CUR_BLOCK (logger_blocked(LOGGER_CUR_FILE))

#if defined LOGGER_BLOCK && defined LOGGER_PREFIX && !defined(LOGGER_DISABLE)
    #undef logger_fatalf
    #define logger_fatalf(fmt, ...)                                            \
        ({                                                                     \
            if (!LOGGER_BLOCK)                                                 \
                logger_fatalf("[%21s] " fmt, LOGGER_PREFIX, ##__VA_ARGS__);    \
        })

    #undef logger_errorf
    #define logger_errorf(fmt, ...)                                            \
        ({                                                                     \
            if (!LOGGER_BLOCK)                                                 \
                logger_errorf("[%21s] " fmt, LOGGER_PREFIX, ##__VA_ARGS__);    \
        })

    #undef logger_warnf
    #define logger_warnf(fmt, ...)                                             \
        ({                                                                     \
            if (!LOGGER_BLOCK)                                                 \
                logger_warnf("[%21s] " fmt, LOGGER_PREFIX, ##__VA_ARGS__);     \
        })

    #undef logger_infof
    #define logger_infof(fmt, ...)                                             \
        ({                                                                     \
            if (!LOGGER_BLOCK)                                                 \
                logger_infof("[%21s] " fmt, LOGGER_PREFIX, ##__VA_ARGS__);     \
        })

    #undef logger_debugf
    #define logger_debugf(fmt, ...)                                            \
        ({                                                                     \
            if (!LOGGER_BLOCK)                                                 \
                logger_debugf("[%21s] " fmt, LOGGER_PREFIX, ##__VA_ARGS__);    \
        })
#endif

#endif
