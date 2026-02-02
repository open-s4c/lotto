#include <string.h>

#define LOGGER_PREFIX LOGGER_CUR_FILE
#define LOGGER_BLOCK  LOGGER_CUR_BLOCK
#include <lotto/brokers/catmgr.h>
#include <lotto/brokers/pubsub.h>
#include <lotto/brokers/statemgr.h>
#include <lotto/engine/dispatcher.h>
#include <lotto/engine/prng.h>
#include <lotto/modules/filtering/state.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger_block.h>
#include <lotto/sys/stdio.h>
#include <lotto/sys/stdlib.h>
#include <lotto/sys/string.h>
#include <lotto/util/macros.h>

/* Amount of dropped events of a category.
 *
 * - p = 1.0: all events of the category
 * - p = 0.0: no event of the category
 *
 * 1.) default all to 0 (nothing dropped)
 * 2.) during init, set some for default filtering
 * 3.) load filtering config file if specified
 */

static double _drop[CAT_END_];
static double _drop_less[CAT_END_];

void
_set_default_filtering()
{
    _drop[CAT_AFTER_AWRITE]    = 1;
    _drop[CAT_AFTER_AREAD]     = 1;
    _drop[CAT_AFTER_XCHG]      = 1;
    _drop[CAT_AFTER_RMW]       = 1;
    _drop[CAT_AFTER_CMPXCHG_S] = 1;
    _drop[CAT_AFTER_CMPXCHG_F] = 1;
    _drop[CAT_AFTER_FENCE]     = 1;
    _drop[CAT_FUNC_EXIT]       = 1;
    _drop[CAT_BEFORE_READ]     = 1;
    _drop[CAT_BEFORE_WRITE]    = 1;
}

STATIC void _load_state();

PS_SUBSCRIBE_INTERFACE(TOPIC_AFTER_UNMARSHAL_CONFIG, {
    _set_default_filtering();
    *_drop_less = *_drop;

#ifdef QLOTTO_ENABLED
    _drop_less[CAT_BEFORE_READ]  = 0;
    _drop_less[CAT_BEFORE_WRITE] = 0;
#endif

    if (filtering_config()->enabled)
        _load_state();
})

static char *config_data         = NULL;
static size_t config_data_length = 0;
#define MAX_CONFIG_ENTRIES 1000

/* Read the logfile information, if available */
static void
_load_configfile(const char *fname)
{
    if (fname == NULL || !fname[0])
        return;
    FILE *fp = fopen(fname, "r");
    if (fp == NULL) {
        logger_debugln("Failed to opened filtering configuration: %s", fname);
        return;
    }
    sys_fseek(fp, 0, SEEK_END);
    config_data_length = sys_ftell(fp);
    config_data        = sys_malloc(config_data_length + 1);
    sys_rewind(fp);
    size_t read_items = sys_fread(config_data, config_data_length, 1, fp);
    ASSERT(read_items == 1 && "reading failed");
    fclose(fp);

    config_data[config_data_length] = 0;
}

char *
deblank(char *input, size_t length)
{
    uint64_t i, j;
    char *output = input;
    for (i = 0, j = 0; i < length; i++, j++) {
        if (input[i] != ' ')
            output[j] = input[i];
        else
            j--;
    }

    if (j < length)
        output[j] = 0;

    return output;
}

#define GEN_CAT(cat)                                                           \
    if (!sys_strncmp(config_data + i, category_str(CAT_##cat),                 \
                     sys_strlen(category_str(CAT_##cat)))) {                   \
        i += sys_strlen(category_str(CAT_##cat));                              \
        if (config_data[i] == '=') {                                           \
            i++;                                                               \
            _drop[CAT_##cat] = atof(config_data + i);                          \
        } else {                                                               \
            i -= sys_strlen(category_str(CAT_##cat));                          \
        }                                                                      \
    }

/* Parses the raw information from the config file and places it in _drop[] */
static void
_parse_config()
{
    if (!config_data)
        return;

    // if (!sys_strncmp(config_data + i, "type=", sys_strlen("type=")))
    //     logger_info[l].type = atoll(config_data + i + 5);

    config_data = deblank(config_data, config_data_length);

    size_t i = 0;
    for (size_t l = 0; l < MAX_CONFIG_ENTRIES; l++, i++) {
        bool skip_line = false;
        for (; config_data[i] != '\n'; i++) {
            if (config_data[i] == '\0')
                return;

            if (skip_line)
                continue;

            if (config_data[i] == '#') {
                skip_line = true;
                continue;
            }

            FOR_EACH_CATEGORY
        }
    }
}
#undef GEN_CAT

#define GEN_CAT(cat)                                                           \
    logger_debugln("%s = %f", category_str(CAT_##cat), _drop[CAT_##cat]);
void
_print_config(){FOR_EACH_CATEGORY}
#undef GEN_CAT

STATIC void _load_state()
{
    _load_configfile(filtering_config()->filename);
    _parse_config();
    _print_config();
}


/*******************************************************************************
 * handler functions
 ******************************************************************************/
STATIC void
_filtering_handle(const context_t *ctx, event_t *e)
{
    if (!filtering_config()->enabled)
        return;

    ASSERT(ctx);
    ASSERT(ctx->id != NO_TASK);
    ASSERT(e);

    double p = _drop[ctx->cat];
    if (e->filter_less) {
        p = _drop_less[ctx->cat];
    }

    if (p == 0)
        return;

    if (p < 1) {
        double pp = prng_real();
        ASSERT(pp <= 1.0);
        if (pp > p)
            return;
    }

    e->next     = ctx->id;
    e->readonly = true;
    e->skip     = true;
}
REGISTER_HANDLER(SLOT_FILTERING, _filtering_handle)
