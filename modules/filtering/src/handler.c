#include <string.h>

#include "state.h"
#include <lotto/engine/prng.h>
#include <lotto/engine/pubsub.h>
#include <lotto/engine/sequencer.h>
#include <lotto/engine/statemgr.h>
#include <lotto/runtime/capture_point.h>
#include <lotto/runtime/ingress_events.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger.h>
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

static double _drop[MAX_TYPES];
static double _drop_less[MAX_TYPES];

void
_set_default_filtering()
{
    //    _drop[EVENT_AFTER_AWRITE]    = 1;
    //    _drop[EVENT_AFTER_AREAD]     = 1;
    //    _drop[EVENT_AFTER_XCHG]      = 1;
    //    _drop[EVENT_AFTER_RMW]       = 1;
    //    _drop[EVENT_AFTER_CMPXCHG_S] = 1;
    //    _drop[EVENT_AFTER_CMPXCHG_F] = 1;
    //    _drop[EVENT_AFTER_FENCE]     = 1;
    //    _drop[EVENT_FUNC_EXIT]       = 1;
    //    _drop[EVENT_BEFORE_READ]     = 1;
    //    _drop[EVENT_BEFORE_WRITE]    = 1;
}

STATIC void _load_state();

static type_id
_type_from_number(const char *value)
{
    unsigned long type = strtoul(value, NULL, 10);
    return type < MAX_TYPES ? (type_id)type : ANY_EVENT;
}

static type_id
_effective_type(const capture_point *cp)
{
    type_id type = cp->type_id;
    ASSERT(type < MAX_TYPES);
    return type;
}

LOTTO_SUBSCRIBE(EVENT_ENGINE__AFTER_UNMARSHAL_CONFIG, {
    _set_default_filtering();
    *_drop_less = *_drop;

#ifdef QLOTTO_ENABLED
    _drop_less[EVENT_BEFORE_READ]  = 0;
    _drop_less[EVENT_BEFORE_WRITE] = 0;
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

/* Parses the raw information from the config file and places it in _drop[] */
static void
_parse_config()
{
    if (!config_data)
        return;

    // if (!sys_strncmp(config_data + i, "type=", sys_strlen("type=")))
    //     logger_info[l].type_id = atoll(config_data + i + 5);

    config_data = deblank(config_data, config_data_length);

    char *line = config_data;
    for (size_t l = 0; l < MAX_CONFIG_ENTRIES && *line; l++) {
        char *next = strchr(line, '\n');
        if (next != NULL) {
            *next = '\0';
        }

        if (*line != '\0' && *line != '#') {
            char *eq = strchr(line, '=');
            if (eq != NULL) {
                *eq          = '\0';
                type_id type = _type_from_number(line);
                if (type != ANY_EVENT) {
                    _drop[type] = atof(eq + 1);
                }
            }
        }

        if (next == NULL) {
            break;
        }
        line = next + 1;
    }
}

static void
_print_config()
{
    for (type_id type = 0; type < MAX_TYPES; type++) {
        if (_drop[type] != 0) {
            logger_debugln("%u = %f", type, _drop[type]);
        }
    }
}

STATIC void
_load_state()
{
    _load_configfile(filtering_config()->filename);
    _parse_config();
    _print_config();
}


/*******************************************************************************
 * handler functions
 ******************************************************************************/
STATIC void
_filtering_handle(const capture_point *cp, event_t *e)
{
    if (!filtering_config()->enabled)
        return;

    ASSERT(cp);
    ASSERT(cp->id != NO_TASK);
    ASSERT(e);

    type_id type = _effective_type(cp);
    double p     = _drop[type];
    if (e->filter_less) {
        p = _drop_less[type];
    }

    if (p == 0)
        return;

    if (p < 1) {
        double pp = prng_real();
        ASSERT(pp <= 1.0);
        if (pp > p)
            return;
    }

    e->next     = cp->id;
    e->readonly = true;
    e->skip     = true;
}
ON_SEQUENCER_CAPTURE(_filtering_handle)
