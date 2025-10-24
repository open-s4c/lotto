/*
 */
#ifndef LOTTO_LOG_H
#define LOTTO_LOG_H

#include <stdbool.h>
#include <stdint.h>

#include <lotto/base/marshable.h>

typedef struct log_s {
    marshable_t m;
    uint64_t tid;
    uint64_t id;
    uint64_t data;
} log_t;

void log_init(log_t *log);
bool log_equals(const log_t *log1, const log_t *log2);
void log_print(const marshable_t *m);

#define MARSHABLE_LOG MARSHABLE_STATIC_PRINTABLE(sizeof(log_t), log_print)

#endif
