#include <string.h>

#define LOGGER_PREFIX LOGGER_CUR_FILE
#define LOGGER_BLOCK  LOGGER_CUR_BLOCK
#include <lotto/base/marshable.h>
#include <lotto/brokers/pubsub.h>
#include <lotto/brokers/statemgr.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger_block.h>
#include <lotto/sys/string.h>

#define CANARY 0xbadfeed

typedef struct {
    marshable_t *m;
} entry_t;

typedef struct {
    entry_t entries[1024];
    size_t length;
} statemgr_t;

static statemgr_t _groups[STATE_TYPE_END_];

typedef struct {
    uint64_t canary;
    size_t index;
    size_t size;
    bool empty;
} header_t;

static char *
_add_header(const void *buf)
{
    char *b = (char *)buf;
    return b + sizeof(header_t);
}

static void
_header_marshal(header_t h, void *buf)
{
    ASSERT(buf);
    h.canary = CANARY;
    sys_memcpy(buf, &h, sizeof(header_t));
}

static header_t
_header_unmarshal(const void *buf)
{
    ASSERT(buf);
    header_t h = {0};
    sys_memcpy(&h, buf, sizeof(header_t));
    if (h.canary != CANARY)
        logger_fatalf("unmarshal error: dead canary\n");

    return h;
}

static void
_statemgr_register(statemgr_t *mgr, marshable_t *m)
{
    size_t index   = mgr->length;
    entry_t *entry = &mgr->entries[index];
    entry->m       = m;
    mgr->length++;
}


void
statemgr_register(marshable_t *m, state_type_t type)
{
    _statemgr_register(&_groups[type], m);
}

static size_t
_statemgr_size(statemgr_t *mgr)
{
    size_t size = 0;
    marshable_t *m;
    for (size_t i = 0; i < mgr->length; i++)
        if ((m = mgr->entries[i].m) != NULL && m->size)
            size += marshable_size(m) + sizeof(header_t);
    return size;
}

size_t
statemgr_size(state_type_t type)
{
    ASSERT(type < STATE_TYPE_END_);
    ASSERT(type != STATE_TYPE_EPHEMERAL);
    return _statemgr_size(&_groups[type]);
}

static const void *
_statemgr_unmarshal(statemgr_t *mgr, const void *buf)
{
    marshable_t *m;
    for (size_t i = 0; i < mgr->length; i++) {
        header_t h = _header_unmarshal(buf);
        if (h.index > i)
            continue;

        ASSERT(h.index == i);

        if ((m = mgr->entries[i].m) == NULL) {
            logger_warnf(
                "unmarshal error: index %zu not registered, "
                "skipping.\n",
                h.index);
            buf = _add_header(buf) + h.size;
            continue;
        }

        char *payload    = _add_header(buf);
        const void *next = marshable_unmarshal(m, payload);
        ASSERT((uintptr_t)next - (uintptr_t)payload == h.size);
        buf = next;
    }
    return buf;
}

const void *
statemgr_unmarshal(const void *buf, state_type_t type, bool publish)
{
    ASSERT(type < STATE_TYPE_END_);
    ASSERT(type != STATE_TYPE_EPHEMERAL);
    ASSERT(buf);
    header_t h = _header_unmarshal(buf);
    if (h.empty)
        return (char *)buf + h.size;
    buf = _statemgr_unmarshal(&_groups[type], buf);

    if (!publish) {
        return buf;
    }

    switch (type) {
        case STATE_TYPE_CONFIG:
            PS_PUBLISH_INTERFACE(TOPIC_AFTER_UNMARSHAL_CONFIG, nil);
            break;
        case STATE_TYPE_PERSISTENT:
            PS_PUBLISH_INTERFACE(TOPIC_AFTER_UNMARSHAL_PERSISTENT, nil);
            break;
        case STATE_TYPE_FINAL:
            PS_PUBLISH_INTERFACE(TOPIC_AFTER_UNMARSHAL_FINAL, nil);
            break;
        default:
            break;
    }

    return buf;
}

static void *
_statemgr_marshal(statemgr_t *mgr, void *buf)
{
    marshable_t *m;
    for (size_t i = 0; i < mgr->length; i++) {
        if ((m = mgr->entries[i].m) != NULL && m->size) {
            char *payload = _add_header(buf);
            void *next    = marshable_marshal(m, payload);
            ASSERT((uintptr_t)next >= (uintptr_t)payload);
            header_t h = {.index = i,
                          .size  = (char *)next - payload,
                          .empty = false};
            _header_marshal(h, buf);
            buf = next;
        }
    }
    return buf;
}

void *
statemgr_marshal(void *buf, state_type_t type)
{
    ASSERT(type < STATE_TYPE_END_);
    ASSERT(type != STATE_TYPE_EPHEMERAL);
    ASSERT(buf);
    if (type == STATE_TYPE_EMPTY) {
        header_t h = {.empty = true, .size = statemgr_size(type)};
        _header_marshal(h, buf);
        return _add_header(buf);
    }

    switch (type) {
        case STATE_TYPE_CONFIG:
            PS_PUBLISH_INTERFACE(TOPIC_BEFORE_MARSHAL_CONFIG, nil);
            break;
        case STATE_TYPE_PERSISTENT:
            PS_PUBLISH_INTERFACE(TOPIC_BEFORE_MARSHAL_PERSISTENT, nil);
            break;
        case STATE_TYPE_FINAL:
            PS_PUBLISH_INTERFACE(TOPIC_BEFORE_MARSHAL_FINAL, nil);
            break;
        default:
            break;
    }
    return _statemgr_marshal(&_groups[type], buf);
}

static void
_statemgr_print(statemgr_t *mgr)
{
    marshable_t *m;
    for (size_t i = 0; i < mgr->length; i++)
        if ((m = mgr->entries[i].m) != NULL) {
            marshable_print(m);
        }
}

void
statemgr_print(state_type_t type)
{
    return _statemgr_print(&_groups[type]);
}

void
statemgr_record_unmarshal(const record_t *r)
{
    switch (r->kind) {
            /* begin of clock cases */
        case RECORD_SCHED:
            statemgr_unmarshal(r->data, STATE_TYPE_PERSISTENT, true);
            break;
        case RECORD_FORCE:
        case RECORD_OPAQUE:
        case RECORD_EXIT:
            break;
        case RECORD_START:
            statemgr_unmarshal(r->data, STATE_TYPE_START, true);
            break;
        case RECORD_CONFIG:
            statemgr_unmarshal(r->data, STATE_TYPE_CONFIG, true);
            break;
        default:
            logger_fatalf("unexpected %s record\n", kind_str(r->kind));
    }
}
