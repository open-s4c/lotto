/*
 */
#ifndef LOTTO_LOGGER_BLOCK_H
#define LOTTO_LOGGER_BLOCK_H

#include <stdbool.h>

#include <lotto/sys/logger.h>

void logger_block_init(char *cats);
bool logger_blocked(const char *file_name);
void logger_block_fini(void);

#define LOG_CUR_BLOCK (logger_blocked(LOG_CUR_FILE))

#if defined LOG_BLOCK && defined LOG_PREFIX && !defined(LOG_DISABLE)
    #undef log_fatalf
    #define log_fatalf(fmt, ...)                                               \
        ({                                                                     \
            if (!LOG_BLOCK)                                                    \
                log_fatalf("[%21s] " fmt, LOG_PREFIX, ##__VA_ARGS__);          \
        })

    #undef log_errorf
    #define log_errorf(fmt, ...)                                               \
        ({                                                                     \
            if (!LOG_BLOCK)                                                    \
                log_errorf("[%21s] " fmt, LOG_PREFIX, ##__VA_ARGS__);          \
        })

    #undef log_warnf
    #define log_warnf(fmt, ...)                                                \
        ({                                                                     \
            if (!LOG_BLOCK)                                                    \
                log_warnf("[%21s] " fmt, LOG_PREFIX, ##__VA_ARGS__);           \
        })

    #undef log_infof
    #define log_infof(fmt, ...)                                                \
        ({                                                                     \
            if (!LOG_BLOCK)                                                    \
                log_infof("[%21s] " fmt, LOG_PREFIX, ##__VA_ARGS__);           \
        })

    #undef log_debugf
    #define log_debugf(fmt, ...)                                               \
        ({                                                                     \
            if (!LOG_BLOCK)                                                    \
                log_debugf("[%21s] " fmt, LOG_PREFIX, ##__VA_ARGS__);          \
        })
#endif

#endif
