/*
 */
#ifndef LOTTO_LOG_H
#define LOTTO_LOG_H
/**
 * @file log.h
 * @brief The trace reconstruction interface.
 *
 * Log points are used to match the execution against a log sequence. Logs have
 * ids and optional data which are observed after the log is executed.
 */

#include <stdint.h>

/**
 * Inserts logging point.
 *
 * @param id id
 */
void lotto_log(const uint64_t id) __attribute__((weak));
/**
 * Inserts logging point with data.
 *
 * @param id id
 * @param data data
 */
void lotto_log_data(const uint64_t id, int64_t data) __attribute__((weak));

#endif
