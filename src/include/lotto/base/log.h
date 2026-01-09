/*
 */
#ifndef LOTTO_LOG_H
#define LOTTO_LOG_H

#include <stdbool.h>
#include <stdint.h>

#include <lotto/base/marshable.h>

typedef struct logger_s {
    marshable_t m;
    uint64_t tid;
    uint64_t id;
    uint64_t data;
} logger_t;

void logger_init(logger_t *log);
bool logger_equals(const logger_t *log1, const logger_t *log2);
void logger_print(const marshable_t *m);

#define MARSHABLE_LOG MARSHABLE_STATIC_PRINTABLE(sizeof(logger_t), logger_print)

#endif
