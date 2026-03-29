#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "../src/parser.h"
#include "../src/state.h"
#include <lotto/base/bag.h>

struct error_state {
    size_t count;
    size_t lines[8];
    const char *msgs[8];
    const char *details[8];
};

static void
_collect_error(size_t line, const char *msg, const char *detail, void *arg)
{
    struct error_state *state = arg;

    assert(state->count < sizeof(state->lines) / sizeof(state->lines[0]));
    state->lines[state->count]   = line;
    state->msgs[state->count]    = msg;
    state->details[state->count] = detail;
    state->count++;
}

static void
test_valid_config(void)
{
    bag_t entries;
    char text[] =
        "# comment\n"
        "\n"
        "EVENT_MA_READ    =     1.0\n"
        "EVENT_MA_AREAD=1\n"
        " EVENT_MA_WRITE=0\n"
        "EVENT_MA_RMW=0.0001\n";
    const struct sampling_config_entry *entry;

    bag_init(&entries, MARSHABLE_STATIC(sizeof(struct sampling_config_entry)));

    bool ok = sampling_parse_config(text, &entries, NULL, NULL);

    assert(ok);
    assert(bag_size(&entries) == 4);

    entry = (const struct sampling_config_entry *)bag_iterate(&entries);
    assert(strcmp(entry->name, "EVENT_MA_RMW") == 0);
    assert(fabs(entry->p - 0.0001) < 1e-12);

    entry = (const struct sampling_config_entry *)bag_next(&entry->bi);
    assert(strcmp(entry->name, "EVENT_MA_WRITE") == 0);
    assert(entry->p == 0.0);

    entry = (const struct sampling_config_entry *)bag_next(&entry->bi);
    assert(strcmp(entry->name, "EVENT_MA_AREAD") == 0);
    assert(entry->p == 1.0);

    entry = (const struct sampling_config_entry *)bag_next(&entry->bi);
    assert(strcmp(entry->name, "EVENT_MA_READ") == 0);
    assert(entry->p == 1.0);

    bag_clear(&entries);
}

static void
test_invalid_config(void)
{
    bag_t entries;
    char text[] =
        "EVENT_MA_READ=-1\n"
        "EVENT_MA_READ=1.1\n"
        "EVENT_MA_READ=-0.1\n"
        " = 1\n";
    struct error_state errors = {0};

    bag_init(&entries, MARSHABLE_STATIC(sizeof(struct sampling_config_entry)));

    bool ok = sampling_parse_config(text, &entries, _collect_error, &errors);

    assert(!ok);
    assert(errors.count == 4);
    assert(errors.lines[0] == 1);
    assert(errors.lines[1] == 2);
    assert(errors.lines[2] == 3);
    assert(errors.lines[3] == 4);
    assert(strcmp(errors.msgs[0], "invalid probability") == 0);
    assert(strcmp(errors.msgs[1], "invalid probability") == 0);
    assert(strcmp(errors.msgs[2], "invalid probability") == 0);
    assert(strcmp(errors.msgs[3], "missing event type") == 0);
    assert(bag_size(&entries) == 0);

    bag_clear(&entries);
}

static void
test_name_too_long(void)
{
    bag_t entries;
    char name[SAMPLING_NAME_MAX_LEN + 2];
    char text[sizeof(name) + 8];
    struct error_state errors = {0};

    memset(name, 'A', sizeof(name) - 1);
    name[sizeof(name) - 1] = '\0';
    snprintf(text, sizeof(text), "%s=1\n", name);

    bag_init(&entries, MARSHABLE_STATIC(sizeof(struct sampling_config_entry)));

    bool ok = sampling_parse_config(text, &entries, _collect_error, &errors);

    assert(!ok);
    assert(errors.count == 1);
    assert(strcmp(errors.msgs[0], "event type name too long") == 0);
    assert(bag_size(&entries) == 0);

    bag_clear(&entries);
}

int
main(void)
{
    test_valid_config();
    test_invalid_config();
    test_name_too_long();
    return 0;
}
