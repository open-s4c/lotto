#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "state.h"

static char *
_trim_left(char *s)
{
    while (*s != '\0' && isspace((unsigned char)*s))
        s++;
    return s;
}

static void
_trim_right(char *s)
{
    size_t len = strlen(s);

    while (len > 0 && isspace((unsigned char)s[len - 1])) {
        s[len - 1] = '\0';
        len--;
    }
}

static void
_report_error(size_t line, const char *msg, const char *detail,
              sampling_parse_error_f report, void *arg)
{
    if (report != NULL)
        report(line, msg, detail, arg);
}

static bool
_parse_probability(const char *raw, double *out)
{
    char *end = NULL;
    double p  = strtod(raw, &end);

    if (raw == end)
        return false;
    while (*end != '\0' && isspace((unsigned char)*end))
        end++;
    if (*end != '\0')
        return false;
    if (p < 0.0 || p > 1.0)
        return false;

    *out = p;
    return true;
}

static bool
_parse_line(char *line, size_t lineno, bag_t *entries,
            sampling_parse_error_f report, void *arg)
{
    char *eq;
    char *name;
    char *value;
    double p;
    struct sampling_config_entry *entry;

    line = _trim_left(line);
    _trim_right(line);
    if (*line == '\0' || *line == '#')
        return true;

    eq = strchr(line, '=');
    if (eq == NULL || strchr(eq + 1, '=') != NULL) {
        _report_error(lineno, "invalid config line", line, report, arg);
        return false;
    }

    *eq   = '\0';
    name  = _trim_left(line);
    value = _trim_left(eq + 1);
    _trim_right(name);
    _trim_right(value);

    if (*name == '\0') {
        _report_error(lineno, "missing event type", "", report, arg);
        return false;
    }
    if (*value == '\0') {
        _report_error(lineno, "missing probability", name, report, arg);
        return false;
    }

    if (strlen(name) > SAMPLING_NAME_MAX_LEN) {
        _report_error(lineno, "event type name too long", name, report, arg);
        return false;
    }

    if (!_parse_probability(value, &p)) {
        _report_error(lineno, "invalid probability", value, report, arg);
        return false;
    }

    entry    = (struct sampling_config_entry *)bag_add(entries);
    entry->p = p;
    strcpy(entry->name, name);
    return true;
}

bool
sampling_parse_config(char *text, bag_t *entries, sampling_parse_error_f report,
                      void *arg)
{
    bool ok    = true;
    char *line = text;

    if (entries == NULL)
        return false;

    if (text == NULL)
        return true;

    bag_clear(entries);

    for (size_t lineno = 1; *line != '\0'; lineno++) {
        char *next = strchr(line, '\n');

        if (next != NULL)
            *next = '\0';

        ok &= _parse_line(line, lineno, entries, report, arg);

        if (next == NULL)
            break;
        line = next + 1;
    }

    if (!ok)
        bag_clear(entries);

    return ok;
}
