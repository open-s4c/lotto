/*
 */
#include <dirent.h>
#include <limits.h>
#include <string.h>

#define LOG_PREFIX LOG_CUR_FILE
#define LOG_BLOCK  LOG_CUR_BLOCK
#include <lotto/base/address_bdd.h>
#include <lotto/base/stable_address.h>
#include <lotto/base/string_hash_table.h>
#include <lotto/base/tidmap.h>
#include <lotto/brokers/pubsub.h>
#include <lotto/brokers/statemgr.h>
#include <lotto/engine/dispatcher.h>
#include <lotto/states/handlers/capture_group.h>
#include <lotto/states/sequencer.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger_block.h>
#include <lotto/sys/memory.h>
#include <lotto/sys/stdio.h>
#include <lotto/sys/stdlib.h>
#include <lotto/sys/stream_file.h>
#include <lotto/sys/string.h>
#include <lotto/util/macros.h>

#define CAPTURED_FUNCTIONS_MAX_LENGTH 10000

typedef struct {
    char *name;
    bool relevant_return;
    bool relevant_memory;
} delayed_function_t;

typedef struct {
    tiditem_t t;
    delayed_function_t *function;
    uint64_t call_depth;
} task_t;

#define MARSHABLE_TASK MARSHABLE_STATIC(sizeof(task_t)) // TODO: broken

typedef struct {
    tiditem_t t;
    uint64_t value;
} return_t;

#define MARSHABLE_RETURN MARSHABLE_STATIC(sizeof(return_t))

typedef struct {
    marshable_t m;
    tidset_t total_order;
    tidmap_t return_map;
    tidmap_t read_map;
} atomic_outcome_t;

typedef struct {
    tiditem_t t;
    uint8_t value;
    stable_address_t address;
    stable_address_t pc;
} memory_value_t;

typedef struct {
    tiditem_t t;
    tidset_t tasks;
} tasks_t;

typedef struct {
    marshable_t m;
    address_bdd_t *write_set;
    atomic_outcome_t atomic_outcome;
    tidset_t finished_tasks; // tasks finished with the capture group in
                             // chronological order
    tidmap_t partial_order;  // task -> predecessors
    char payload[0];
    bool in_group;
    bool first_group;
    task_id current_task;
    tidmap_t map;
} state_t;
static state_t _state;

/*******************************************************************************
 * delayed functions
 ******************************************************************************/
typedef struct {
    tiditem_t t;
    delayed_function_t function;
} delayed_function_item_t;

#define MARSHABLE_FUNCTION                                                     \
    MARSHABLE_STATIC(sizeof(delayed_function_item_t)) // TODO: broken

static tidmap_t delayed_function_table;
static char *delayed_functions_copy   = NULL;
static char *delayed_functions_backup = NULL;
static size_t delayed_functions_length;

static char *_match;
static bool
_name_matches(const tiditem_t *item)
{
    return strcmp(_match, ((delayed_function_item_t *)item)->function.name) ==
           0;
}


static void _filter_active(tidset_t *tset);
static void _reset_group(event_t *e);
static void _initialize_delayed_function_table(const char *delayed_functions);

/*******************************************************************************
 * marshaling interface
 ******************************************************************************/
STATIC size_t _capture_group_size(const marshable_t *m);
STATIC void *_capture_group_marshal(const marshable_t *m, void *buf);
STATIC const void *_capture_group_unmarshal(marshable_t *m, const void *buf);
STATIC void _capture_group_print(const marshable_t *m);

#define MARSHABLE_STATE                                                        \
    (marshable_t)                                                              \
    {                                                                          \
        .alloc_size = sizeof(state_t), .unmarshal = _capture_group_unmarshal,  \
        .marshal = _capture_group_marshal, .size = _capture_group_size,        \
        .print = _capture_group_print                                          \
    }

/*******************************************************************************
 * tasks marshaling interface
 ******************************************************************************/
STATIC size_t _ts_size(const marshable_t *m);
STATIC void *_ts_marshal(const marshable_t *m, void *buf);
STATIC const void *_ts_unmarshal(marshable_t *m, const void *buf);

#define MARSHABLE_TASKS                                                        \
    (marshable_t)                                                              \
    {                                                                          \
        .alloc_size = sizeof(tasks_t), .unmarshal = _ts_unmarshal,             \
        .marshal = _ts_marshal, .size = _ts_size                               \
    }

/*******************************************************************************
 * atomic outcome marshaling interface
 ******************************************************************************/
STATIC size_t _ao_size(const marshable_t *m);
STATIC void *_ao_marshal(const marshable_t *m, void *buf);
STATIC const void *_ao_unmarshal(marshable_t *m, const void *buf);
STATIC void _ao_print(const marshable_t *m);

#define MARSHABLE_ATOMIC_OUTCOME                                               \
    (marshable_t)                                                              \
    {                                                                          \
        .alloc_size = sizeof(atomic_outcome_t), .unmarshal = _ao_unmarshal,    \
        .marshal = _ao_marshal, .size = _ao_size, .print = _ao_print           \
    }

/*******************************************************************************
 * init and printing
 ******************************************************************************/
PS_SUBSCRIBE_INTERFACE(TOPIC_AFTER_UNMARSHAL_CONFIG, {
    tidmap_init(&delayed_function_table, MARSHABLE_FUNCTION);
    _initialize_delayed_function_table(capture_group_config()->functions);
})
PS_SUBSCRIBE_INTERFACE(TOPIC_DELAYED_PATH, {
    ASSERT(sys_strlen(as_sval(v)) < PATH_MAX);
    strcpy(capture_group_config()->path, as_sval(v));
    capture_group_config()->check = true;
})
static void
_init()
{
    _state.m = MARSHABLE_STATE;
    tidmap_init(&_state.map, MARSHABLE_TASK);
    _state.first_group = true;
    _state.write_set   = address_bdd_new(64);
    tidset_init(&_state.finished_tasks);
    tidmap_init(&_state.partial_order, MARSHABLE_TASKS);
    _state.atomic_outcome.m = MARSHABLE_ATOMIC_OUTCOME;
    tidmap_init(&_state.atomic_outcome.read_map,
                MARSHABLE_STATIC(sizeof(memory_value_t)));
    tidmap_init(&_state.atomic_outcome.return_map, MARSHABLE_RETURN);
    tidset_init(&_state.atomic_outcome.total_order);
}
REGISTER_EPHEMERAL(_state, { _init(); })

static void
_initialize_delayed_function_table(const char *delayed_functions)
{
    tidmap_clear(&delayed_function_table);
    if (!delayed_functions) {
        delayed_functions = "";
    }
    if (delayed_functions_backup) {
        sys_free(delayed_functions_backup);
    }
    delayed_functions_backup = sys_strdup(delayed_functions);
    if (delayed_functions_copy) {
        sys_free(delayed_functions_copy);
    }
    delayed_functions_copy = sys_strdup(delayed_functions);
    char *pt_state;
    for (char *pt       = strtok_r(delayed_functions_copy, ",", &pt_state);
         pt != NULL; pt = strtok_r(NULL, ",", &pt_state)) {
        delayed_function_item_t *df =
            (delayed_function_item_t *)tidmap_register(
                &delayed_function_table,
                tidmap_size(&delayed_function_table) + 1);
        char *elem_pt_state;
        char *elem_pt = strtok_r(pt, ":", &elem_pt_state);

        ASSERT(elem_pt);
        df->function.name = elem_pt;
        elem_pt           = strtok_r(NULL, ":", &elem_pt_state);
        ASSERT(elem_pt);
        df->function.relevant_return = elem_pt[0] == '1';
        elem_pt                      = strtok_r(NULL, ":", &elem_pt_state);
        ASSERT(elem_pt);
        df->function.relevant_memory = elem_pt[0] == '1';
    }
}

static void
_filter_active(tidset_t *tset)
{
    /* filter blocked tasks */
    (_state.in_group ? tidset_retain_all_keys :
                       tidset_remove_all_keys)(tset, &_state.map);
}

static void
_reset_group(event_t *e)
{
    if (capture_group_config()->atomic && capture_group_config()->check &&
        tidmap_size(&_state.map)) {
        log_println("[lotto] atomic capture group broken, terminating");
        e->reason = REASON_IGNORE;
        return;
    }
    tidmap_clear(&_state.map);
    _state.in_group     = false;
    _state.current_task = NO_TASK;
    _state.first_group  = false;
}

static bool
_total_order_satisfies_partial_order(const tidset_t *total_order,
                                     const tidmap_t *partial_order)
{
    for (size_t i = 0; i < tidset_size(total_order); i++) {
        task_id t   = tidset_get(total_order, i);
        tasks_t *ts = (tasks_t *)tidmap_find(partial_order, t);
        if (!ts) {
            continue;
        }
        // for each predecessor find it the prefix of the finished tasks
        for (size_t j = 0; j < tidset_size(&ts->tasks); j++) {
            task_id tid = tidset_get(&ts->tasks, j);
            bool found  = false;
            for (size_t k = 0; k < i; k++) {
                if ((found = tidset_get(total_order, k) == tid)) {
                    break;
                }
            }
            // if the predecessor is not found, the partial order is violated
            if (!found) {
                return false;
            }
        }
    }
    return true;
}


/*******************************************************************************
 * handler
 ******************************************************************************/
STATIC void
_capture_group_handle(const context_t *ctx, event_t *e)
{
    if (capture_group_config()->group_threshold == 0) {
        return;
    }
    ASSERT(ctx);
    ASSERT(ctx->id != NO_TASK);
    ASSERT(e);

    task_t *t = (task_t *)tidmap_find(&_state.map, ctx->id);
    if (_state.in_group) {
        if (_state.current_task == NO_TASK) {
            // the next task in the group
            _state.current_task = ctx->id;
        }
        if (!tidmap_find(&_state.partial_order, ctx->id)) {
            // when the task begins the capture group execution, add a mapping
            // from it to its predecessors
            tasks_t *tasks =
                (tasks_t *)tidmap_register(&_state.partial_order, ctx->id);
            tidset_init(&tasks->tasks);
            tidset_copy(&tasks->tasks, &_state.finished_tasks);
        }
    }
    if ((t != NULL) != _state.in_group ||
        (_state.in_group && capture_group_config()->atomic &&
         _state.current_task != ctx->id)) {
        // capture group is broken, reset group
        _reset_group(e);
        if (IS_REASON_TERMINATE(e->reason)) {
            return;
        }
        t = NULL;
    }

    bool write = false;
    bool read  = false;

    switch (ctx->cat) {
        case CAT_FUNC_ENTRY:
            if (!_state.first_group) {
                break;
            }
            if (_state.in_group) {
                t->call_depth++;
                break;
            }
            _match                       = (char *)ctx->args[1].value.ptr;
            delayed_function_item_t *dfi = NULL;
            if (_state.in_group || t || !_match ||
                tidmap_size(&_state.map) >=
                    capture_group_config()->group_threshold ||
                !(dfi = (delayed_function_item_t *)tidmap_find_first(
                      &delayed_function_table, _name_matches))) {
                break;
            }
            t             = (task_t *)tidmap_register(&_state.map, ctx->id);
            t->call_depth = 0;
            t->function   = &dfi->function;
            e->is_chpt    = true;

            if ((_state.in_group = tidmap_size(&_state.map) ==
                                   capture_group_config()->group_threshold)) {
                // start the capture group
                _state.current_task = NO_TASK;
                if (e->selector == SELECTOR_UNDEFINED) {
                    e->selector = SELECTOR_RANDOM;
                }
            }
            break;
        case CAT_FUNC_EXIT:
            if (!_state.first_group || !_state.in_group) {
                break;
            }
            if (t->call_depth > 0) {
                t->call_depth--;
                break;
            }
            // exit the captured function
            tidmap_deregister(&_state.map, ctx->id);
            // switch to the next captured task
            _state.current_task = NO_TASK;
            // mark the task as finished the capture group
            tidset_insert(&_state.finished_tasks, ctx->id);
            // record the return value
            if (t->function->relevant_return) {
                return_t *r = (return_t *)tidmap_register(
                    &_state.atomic_outcome.return_map, ctx->id);
                r->value = ctx->args[0].value.u64;
            }
            if (tidmap_size(&_state.map) == 0) {
                // capture group is over
                _reset_group(e);
                if (IS_REASON_TERMINATE(e->reason) ||
                    (!capture_group_config()->atomic &&
                     capture_group_config()->check)) {
                    e->reason = REASON_IGNORE;
                    return;
                }
            } else {
                e->is_chpt = true;
                if (e->selector == SELECTOR_UNDEFINED) {
                    e->selector = SELECTOR_RANDOM;
                }
            }
            break;

        case CAT_BEFORE_WRITE:
        case CAT_BEFORE_AWRITE:
            write = true;
            break;
        case CAT_BEFORE_CMPXCHG:
        case CAT_BEFORE_XCHG:
        case CAT_BEFORE_RMW:
            write = true;
            read  = true;
            break;
        case CAT_BEFORE_AREAD:
        case CAT_BEFORE_READ:
            read = true;
            break;

        default:
            break;
    }
    if (_state.in_group) {
        if (write && t->function->relevant_memory) {
            address_bdd_set(_state.write_set, (void *)ctx->args[0].value.ptr,
                            ctx->args[1].value.u64);
        }
    } else if (!_state.first_group) {
        if (read && address_bdd_get_and(_state.write_set,
                                        (void *)ctx->args[0].value.ptr,
                                        ctx->args[1].value.u64)) {
            for (uint64_t i = 0; i < ctx->args[1].value.u64; i++) {
                memory_value_t *memory_value =
                    (memory_value_t *)tidmap_register(
                        &_state.atomic_outcome.read_map,
                        tidmap_size(&_state.atomic_outcome.read_map) + 1);
                ASSERT(memory_value);

                memory_value->address = stable_address_get(
                    ctx->args[0].value.ptr + i,
                    sequencer_config()->stable_address_method);
                memory_value->value = *((char *)ctx->args[0].value.ptr + i);
                memory_value->pc    = stable_address_get(
                       ctx->pc, sequencer_config()->stable_address_method);
            }
            address_bdd_unset(_state.write_set, (void *)ctx->args[0].value.ptr,
                              ctx->args[1].value.u64);
        }
        if (write) {
            address_bdd_unset(_state.write_set, (void *)ctx->args[0].value.ptr,
                              ctx->args[1].value.u64);
        }
    }
    if (e->readonly || e->skip) {
        return;
    }
    _filter_active(&e->tset);

    if (!capture_group_config()->atomic || !_state.in_group ||
        e->selector != SELECTOR_UNDEFINED || !tidset_has(&e->tset, ctx->id)) {
        return;
    }
    e->selector = SELECTOR_FIRST;
    e->readonly = true;
    tidset_make_first(&e->tset, ctx->id);
}
REGISTER_HANDLER(SLOT_CAPTURE_GROUP, _capture_group_handle);

/*******************************************************************************
 * marshaling implementation
 ******************************************************************************/
STATIC size_t
_capture_group_size(const marshable_t *m)
{
    ASSERT(m);
    state_t *s = (state_t *)m;
    return sizeof(state_t) + marshable_size_m(&s->map) +
           delayed_functions_length + 1;
}

STATIC void *
_capture_group_marshal(const marshable_t *m, void *buf)
{
    ASSERT(m);
    ASSERT(buf);

    char *b    = (char *)buf;
    state_t *s = (state_t *)m;

    sys_memcpy(b, s, sizeof(state_t));
    b += sizeof(state_t);

    b = marshable_marshal_m(&s->map, b);

    strcpy(b, delayed_functions_backup);

    return b + delayed_functions_length + 1;
}


STATIC const void *
_capture_group_unmarshal(marshable_t *m, const void *buf)
{
    ASSERT(m);
    ASSERT(buf);

    size_t off = offsetof(state_t, payload);
    sys_memcpy(m->payload, buf + off, m->alloc_size - off);
    const char *b = (const char *)buf + m->alloc_size;

    state_t *s = (state_t *)m;
    tidmap_init(&s->map, MARSHABLE_TASK);
    b = marshable_unmarshal_m(&s->map, b);
    _initialize_delayed_function_table((char *)b);
    return b + delayed_functions_length + 1;
}

STATIC void
_capture_group_print(const marshable_t *m)
{
    state_t *s = (state_t *)m;
    log_infoln("target functions                 = [%s]",
               delayed_functions_backup);
    log_infoln("accumulated group size           = %lu", tidmap_size(&s->map));
    log_infoln("executing group                  = %s",
               s->in_group ? "yes" : "no");
    if (s->in_group) {
        log_infoln("current task in group          = %lu", s->current_task);
    }
    log_infof("tasks in group with call depth = [");
    const tiditem_t *cur = tidmap_iterate(&s->map);
    bool first           = true;
    for (; cur; cur = tidmap_next(cur)) {
        if (!first)
            log_printf(", ");
        first     = false;
        task_t *t = (task_t *)cur;
        log_printf("%lu:%lu", cur->key, t->call_depth);
    }
    log_println("]");
}

/*******************************************************************************
 * tasks marshaling implementation
 ******************************************************************************/
STATIC size_t
_ts_size(const marshable_t *m)
{
    ASSERT(m);
    tasks_t *ts = (tasks_t *)m;
    return sizeof(tiditem_t) + marshable_size_m(&ts->tasks);
}

STATIC void *
_ts_marshal(const marshable_t *m, void *buf)
{
    ASSERT(m);
    ASSERT(buf);

    char *b     = (char *)buf;
    tasks_t *ts = (tasks_t *)m;

    sys_memcpy(b, ts, sizeof(tasks_t));
    b += sizeof(tasks_t);

    return marshable_marshal_m(&ts->tasks, b);
}


STATIC const void *
_ts_unmarshal(marshable_t *m, const void *buf)
{
    ASSERT(m);
    ASSERT(buf);

    size_t off = offsetof(marshable_t, payload);
    sys_memcpy(m->payload, buf + off, m->alloc_size - off);
    const char *b = (const char *)buf + m->alloc_size;

    tasks_t *ts = (tasks_t *)m;
    tidset_init(&ts->tasks);
    return marshable_unmarshal_m(&ts->tasks, b);
}

/*******************************************************************************
 * atomic outcome implementation
 ******************************************************************************/
STATIC size_t
_ao_size(const marshable_t *m)
{
    ASSERT(m);
    atomic_outcome_t *ao = (atomic_outcome_t *)m;
    return sizeof(atomic_outcome_t) + marshable_size_m(&ao->total_order) +
           marshable_size_m(&ao->return_map) + marshable_size_m(&ao->read_map);
}

STATIC void *
_ao_marshal(const marshable_t *m, void *buf)
{
    ASSERT(m);
    ASSERT(buf);

    char *b              = (char *)buf;
    atomic_outcome_t *ao = (atomic_outcome_t *)m;

    sys_memcpy(b, ao, sizeof(atomic_outcome_t));
    b += sizeof(atomic_outcome_t);

    return marshable_marshal_m(
        &ao->read_map,
        marshable_marshal_m(&ao->return_map,
                            marshable_marshal_m(&ao->total_order, b)));
}


STATIC const void *
_ao_unmarshal(marshable_t *m, const void *buf)
{
    ASSERT(m);
    ASSERT(buf);

    size_t off = offsetof(marshable_t, payload);
    sys_memcpy(m->payload, buf + off, m->alloc_size - off);
    const char *b = (const char *)buf + m->alloc_size;

    atomic_outcome_t *ao = (atomic_outcome_t *)m;
    tidset_init(&ao->total_order);
    tidmap_init(&ao->return_map, MARSHABLE_RETURN);
    tidmap_init(&ao->read_map, MARSHABLE_STATIC(sizeof(memory_value_t)));
    return marshable_unmarshal_m(
        &ao->read_map,
        marshable_unmarshal_m(&ao->return_map,
                              marshable_unmarshal_m(&ao->total_order, b)));
}

STATIC void
_ao_print(const marshable_t *m)
{
    ASSERT(m);
    atomic_outcome_t *ao = (atomic_outcome_t *)m;
    log_printf("total order: ");
    tidset_print(&ao->total_order.m);
    log_printf("returns: [");
    const tiditem_t *cur = tidmap_iterate(&ao->return_map);
    bool first           = true;
    for (; cur; cur = tidmap_next(cur)) {
        if (!first)
            log_printf(", ");
        first       = false;
        return_t *r = (return_t *)cur;
        log_printf("%lu:%lu", cur->key, r->value);
    }
    log_printf("]\n");
    log_printf("memory_map: [");
    cur = tidmap_iterate(&ao->read_map);
    for (; cur; cur = tidmap_next(cur)) {
        memory_value_t *mv = (memory_value_t *)cur;
        char addr[STABLE_ADDRESS_MAX_STR_LEN];
        char pc[STABLE_ADDRESS_MAX_STR_LEN];
        if (stable_address_sprint(&mv->address, addr) >=
            STABLE_ADDRESS_MAX_STR_LEN)
            ASSERT(0 && "unexpected sprint error");

        if (stable_address_sprint(&mv->pc, pc) >= STABLE_ADDRESS_MAX_STR_LEN)
            ASSERT(0 && "unexpected sprint error");
        log_printf("\n%s -> %02x @ %s", addr, mv->value, pc);
    }
    log_printf("\n]\n");
}

/*******************************************************************************
 * atomic outcome util
 ******************************************************************************/

static bool
_atomic_outcome_order_satisfied(atomic_outcome_t *atomic_outcome)
{
    return _total_order_satisfies_partial_order(&atomic_outcome->total_order,
                                                &_state.partial_order);
}

static bool
_atomic_outcome_data_satisfied(atomic_outcome_t *atomic_outcome)
{
    bool result = true;
    for (const tiditem_t *cur = tidmap_iterate(&atomic_outcome->return_map);
         cur && result; cur   = tidmap_next(cur)) {
        for (const tiditem_t *wcur =
                 tidmap_iterate(&_state.atomic_outcome.return_map);
             wcur && result; wcur = tidmap_next(wcur)) {
            if (cur->key != wcur->key) {
                continue;
            }
            return_t *r  = (return_t *)cur;
            return_t *wr = (return_t *)wcur;
            result       = r->value == wr->value;
            break;
        }
    }
    for (const tiditem_t *cur = tidmap_iterate(&atomic_outcome->read_map);
         cur && result; cur   = tidmap_next(cur)) {
        memory_value_t *v = (memory_value_t *)cur;
        for (const tiditem_t *wcur =
                 tidmap_iterate(&_state.atomic_outcome.read_map);
             wcur && result; wcur = tidmap_next(wcur)) {
            memory_value_t *wv = (memory_value_t *)wcur;
            if (!stable_address_equals(&v->address, &wv->address)) {
                continue;
            }
            result = v->value == wv->value;
            break;
        }
    }
    return result;
}

static void
_atomic_outcome_diff_print(atomic_outcome_t *atomic_outcome)
{
    log_printf("total order: ");
    tidset_print(&atomic_outcome->total_order.m);
    log_println("return map:");
    for (const tiditem_t *cur = tidmap_iterate(&atomic_outcome->return_map);
         cur; cur             = tidmap_next(cur)) {
        for (const tiditem_t *wcur =
                 tidmap_iterate(&_state.atomic_outcome.return_map);
             wcur; wcur = tidmap_next(wcur)) {
            if (cur->key != wcur->key) {
                continue;
            }
            return_t *r  = (return_t *)cur;
            return_t *wr = (return_t *)wcur;
            log_printf("%lu -> %lu", cur->key, r->value);
            if (r->value != wr->value) {
                log_println(" != %lu", wr->value);
            } else {
                log_println();
            }
            break;
        }
    }
    log_println("memory_map:");
    for (const tiditem_t *cur = tidmap_iterate(&atomic_outcome->read_map); cur;
         cur                  = tidmap_next(cur)) {
        memory_value_t *v = (memory_value_t *)cur;
        for (const tiditem_t *wcur =
                 tidmap_iterate(&_state.atomic_outcome.read_map);
             wcur; wcur = tidmap_next(wcur)) {
            memory_value_t *wv = (memory_value_t *)wcur;
            if (!stable_address_equals(&v->address, &wv->address)) {
                continue;
            }
            char addr[STABLE_ADDRESS_MAX_STR_LEN];
            char pc[STABLE_ADDRESS_MAX_STR_LEN];
            if (stable_address_sprint(&v->address, addr) >=
                STABLE_ADDRESS_MAX_STR_LEN)
                ASSERT(0);
            if (stable_address_sprint(&v->pc, pc) >= STABLE_ADDRESS_MAX_STR_LEN)
                ASSERT(0);
            log_printf("%s @ %s -> 0x%02x", addr, pc, v->value);
            if (v->value != wv->value) {
                log_println(" != 0x%02x", wv->value);
            } else {
                log_println();
            }
            break;
        }
    }
}

/*******************************************************************************
 * memory state dump/verification
 ******************************************************************************/
#define mask 0x00fff
static __attribute__((destructor)) void
_lib_destroy()
{
    if (!capture_group_config()->check) {
        return;
    }
    if (capture_group_config()->atomic) {
        if (_state.first_group) {
            log_println("[lotto] no capture group detected");
            return;
        }
        tidset_copy(&_state.atomic_outcome.total_order, &_state.finished_tasks);
        size_t size = marshable_size_m(&_state.atomic_outcome);
        char *buf   = sys_malloc(size);
        marshable_marshal_m(&_state.atomic_outcome, buf);
        stream_t *stream = stream_file_alloc();
        stream_file_out(stream, capture_group_config()->path);
        if (stream_write(stream, buf, size) != size)
            log_fatalf("could not write to stream");
        sys_free(buf);
        stream_close(stream);
        sys_free(stream);
        return;
    }
    address_bdd_iterator_t *i = address_bdd_iterator(_state.write_set);
    for (uintptr_t addr = address_bdd_iterator_next(i); addr;
         addr           = address_bdd_iterator_next(i)) {
        memory_value_t *memory_value = (memory_value_t *)tidmap_register(
            &_state.atomic_outcome.read_map,
            tidmap_size(&_state.atomic_outcome.read_map) + 1);
        memory_value->address =
            stable_address_get(addr, sequencer_config()->stable_address_method);
        memory_value->value = *(char *)addr;
    }
    address_bdd_iterator_free(i);
    DIR *dp = opendir(capture_group_config()->path);
    ASSERT(dp);
    bool order_found = false;
    for (struct dirent *file; (file = readdir(dp));) {
        if (!strcmp(file->d_name, ".") || !strcmp(file->d_name, ".."))
            continue;
        char filename[PATH_MAX];
        sys_sprintf(filename, "%s/%s", capture_group_config()->path,
                    file->d_name);
        FILE *fp = fopen(filename, "r");
        ASSERT(fp);
        sys_fseek(fp, 0, SEEK_END);
        long size = sys_ftell(fp);
        char *buf = sys_malloc(size);
        ASSERT(buf != NULL);
        sys_rewind(fp);
        size_t num_read = sys_fread(buf, size, 1, fp);
        ASSERT(num_read == 1 && "failed to read");
        atomic_outcome_t atomic_outcome;
        atomic_outcome.m = MARSHABLE_ATOMIC_OUTCOME;
        marshable_unmarshal_m(&atomic_outcome, buf);
        fclose(fp);
        bool order_sat = _atomic_outcome_order_satisfied(&atomic_outcome);
        order_found |= order_sat;
        if (!order_sat) {
            continue;
        }
        if (_atomic_outcome_data_satisfied(&atomic_outcome)) {
            return;
        }
    }
    if (!order_found) {
        log_println(
            "[lotto] no atomic execution satisfies the nonatomic partial "
            "order, try more --sc-exploration-rounds");
    } else {
        log_println("State diffs:");
        rewinddir(dp);
        for (struct dirent *file; (file = readdir(dp));) {
            if (!strcmp(file->d_name, ".") || !strcmp(file->d_name, ".."))
                continue;
            char filename[PATH_MAX];
            sys_sprintf(filename, "%s/%s", capture_group_config()->path,
                        file->d_name);
            FILE *fp = fopen(filename, "r");
            ASSERT(fp);
            sys_fseek(fp, 0, SEEK_END);
            long size = sys_ftell(fp);
            char *buf = sys_malloc(size);
            sys_rewind(fp);
            size_t num_read = sys_fread(buf, size, 1, fp);
            ASSERT(num_read == 1 && "failed to read");
            atomic_outcome_t atomic_outcome;
            atomic_outcome.m = MARSHABLE_ATOMIC_OUTCOME;
            marshable_unmarshal_m(&atomic_outcome, buf);
            fclose(fp);
            bool order_sat = _atomic_outcome_order_satisfied(&atomic_outcome);
            if (!order_sat) {
                continue;
            }
            _atomic_outcome_diff_print(&atomic_outcome);
        }
    }
    ASSERT(0 && "could not find a corresponding sequential state");
}
