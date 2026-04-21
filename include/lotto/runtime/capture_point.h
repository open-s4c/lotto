/**
 * @file capture_point.h
 * @brief Lotto ingress capture-point declarations.
 */
#ifndef LOTTO_RUNTIME_CAPTURE_POINT_H
#define LOTTO_RUNTIME_CAPTURE_POINT_H

#include <pthread.h>
#include <stdbool.h>

#include <dice/chains/capture.h>
#include <dice/chains/intercept.h>
#include <dice/types.h>
#include <lotto/base/task_id.h>

struct sequencer_decision;

typedef struct source_event {
    const void *pc;
} source_event;

typedef struct capture_task_init_event {
    uintptr_t thread;
    bool detached;
} capture_task_init_event;

typedef struct capture_task_fini_event {
    void *ptr;
} capture_task_fini_event;

typedef struct capture_task_create_event {
    void *thread;
    const void *attr;
    void *run;
} capture_task_create_event;

typedef struct capture_task_detach_event {
    uintptr_t thread;
    int *ret;
} capture_task_detach_event;

typedef struct capture_key_create_event {
    pthread_key_t *key;
    void (*destructor)(void *);
} capture_key_create_event;

typedef struct capture_key_delete_event {
    pthread_key_t key;
} capture_key_delete_event;

typedef struct capture_set_specific_event {
    pthread_key_t key;
    const void *value;
} capture_set_specific_event;

typedef struct capture_point {
    chain_id chain_id; ///< Ingress phase carried from ingress chains
    type_id type_id;   ///< Normalized source event type
    bool blocking;     ///< Whether this semantic ingress event can block
    task_id id;        ///< Task ID
    task_id vid;       ///< Virtual task ID (NO_TASK if not available)
    uintptr_t pc;      ///< Program counter at the interception
    uint64_t cpu_cost; ///< CPU cost accumulated since previous capture point
#if defined(QLOTTO_ENABLED)
    uint32_t pstate; ///< Processor state (EL at bits 2-3)
#endif
    const char *func; ///< Debugging information for the intercepted operation
    uintptr_t func_addr; ///< Address associated with the intercepted operation
    struct sequencer_decision *decision; ///< Sequencer-only decision context
    union {
        void *payload;
        source_event *source;
        capture_task_init_event *task_init;
        capture_task_fini_event *task_fini;
        capture_task_create_event *task_create;
        capture_task_detach_event *task_detach;
        capture_key_create_event *key_create;
        capture_key_delete_event *key_delete;
        capture_set_specific_event *set_specific;
    };
} capture_point;

#define CP_PAYLOAD(ev) ((__typeof__(ev))cp->payload)

#endif
