/*
 */
#ifndef LOTTO_RECORD_H
#define LOTTO_RECORD_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <lotto/base/category.h>
#include <lotto/base/clk.h>
#include <lotto/base/reason.h>
#include <lotto/base/task_id.h>

/**
 * Defines the kind of record and its payload type (if any).
 */
enum record {
    RECORD_NONE          = 0x00U,
    RECORD_INFO          = 0x01U,
    RECORD_SCHED         = 0x02U,
    RECORD_START         = 0x04U,
    RECORD_EXIT          = 0x08U,
    RECORD_CONFIG        = 0x10U,
    RECORD_SHUTDOWN_LOCK = 0x20U,
    RECORD_OPAQUE        = 0x40U,
    RECORD_FORCE         = 0x80U,
    _RECORD_LAST         = 0x80U,
};
typedef uint64_t kinds_t;

/**
 * Selector for any kind of record.
 */
#define RECORD_ANY (~(0U))

/**
 * Returns a string representing the kind of record.
 */
const char *kind_str(enum record r);

typedef struct record_s {
    struct record_s *next; /**< Shall not be used. */
    task_id id;            /**< Thread creating record. */
    clk_t clk;             /**< Clock value of record. */
    category_t cat;        /**< Context category of task call. */
    reason_t reason;       /**< Reason for record. */
    enum record kind;      /**< Kind of payload: INFO, SCHED, CALL */
    size_t size;           /**< Size of payload. */
    uintptr_t pc;          /**< Programme counter. */
    char data[];           /**< Payload content. */
} record_t;

/**
 * Allocates a new record with a payload of the given size.
 *
 * @param size payload size
 * @return pointer to record if successful, otherwise NULL
 */
record_t *record_alloc(size_t size);

/**
 * Clones the record.
 *
 * @param record record to be copied, must not be NULL
 * @return pointer to a record clone
 */
record_t *record_clone(const record_t *record);

#endif
