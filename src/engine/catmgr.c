#define LOGGER_PREFIX LOGGER_CUR_FILE
#define LOGGER_BLOCK  LOGGER_CUR_BLOCK
#include <lotto/base/marshable.h>
#include <lotto/engine/catmgr.h>
#include <lotto/engine/statemgr.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger_block.h>
#include <lotto/sys/string.h>
#include <lotto/util/macros.h>

#define MAX_CATEGORIES 1024
#define INIT_NEXT      (CAT_END_ + 1) // TODO: remove +1

static const char *_categories[MAX_CATEGORIES];
static category_t _next = INIT_NEXT;

/*******************************************************************************
 * state
 ******************************************************************************/
#define MARSHABLE_STATE                                                        \
    ((marshable_t){                                                            \
        .alloc_size = sizeof(marshable_t),                                     \
        .unmarshal  = _catmgr_unmarshal,                                       \
        .marshal    = _catmgr_marshal,                                         \
        .size       = _catmgr_size,                                            \
        .print      = _catmgr_print,                                           \
    })

STATIC size_t _catmgr_size(const marshable_t *m);
STATIC void *_catmgr_marshal(const marshable_t *m, void *buf);
STATIC const void *_catmgr_unmarshal(marshable_t *m, const void *buf);
STATIC void _catmgr_print(const marshable_t *m);
static marshable_t _m;
static void LOTTO_CONSTRUCTOR
_catmgr_register(void)
{
    _m = MARSHABLE_STATE;
    statemgr_register(DICE_MODULE_SLOT, &_m, STATE_TYPE_CONFIG);
}

STATIC size_t
_catmgr_size(const marshable_t *m)
{
    size_t result = 0;
    for (category_t i = 0; i < _next - INIT_NEXT; i++) {
        result += sys_strlen(_categories[i]) + 1;
    }
    return result + 1;
}
STATIC void *
_catmgr_marshal(const marshable_t *m, void *buf)
{
    char *b = (char *)buf;
    for (category_t i = 0; i < _next - INIT_NEXT; i++) {
        strcpy(b, _categories[i]);
        b += sys_strlen(_categories[i]) + 1;
    }
    strcpy(b, "");
    return b + 1;
}
STATIC const void *
_catmgr_unmarshal(marshable_t *m, const void *buf)
{
    const char *b = (const char *)buf;
    for (category_t i = 0; i < _next - INIT_NEXT; i++) {
        if (strcmp(b, _categories[i]) == 0) {
            b += sys_strlen(_categories[i]) + 1;
            continue;
        }
        logger_errorf("registered category mismatch!\n");
        logger_errorf("actual: [%u, %s]\n", i + INIT_NEXT, _categories[i]);
        logger_errorf("record: [%u, %s]\n", i + INIT_NEXT, (const char *)b);
        _catmgr_print(&_m);
        logger_fatalf();
    }
    return b + 1;
}
STATIC void
_catmgr_print(const marshable_t *m)
{
    logger_infof("registered categories:\n");
    for (category_t i = 0; i < _next - INIT_NEXT; i++) {
        logger_infof("  %u: %s\n", i + INIT_NEXT, _categories[i]);
    }
}

/*******************************************************************************
 * public interface
 ******************************************************************************/

category_t
new_category(const char *name)
{
    ASSERT(name);
    ASSERT(sys_strlen(name) > 0);
    category_t i = _next - INIT_NEXT;
    ASSERT(i < MAX_CATEGORIES);
    _categories[i] = name;
    return _next++;
}

const char *
category_str(category_t category)
{
    if (category < CAT_END_) {
        return base_category_str(category);
    }
    if (category < _next) {
        const char *name = _categories[category - INIT_NEXT];
        if (name != NULL) {
            return name;
        }
    }
    logger_warnf("unknown category id: %u (next: %u)\n", category, _next);
    return "CAT_UNKNOWN";
}
