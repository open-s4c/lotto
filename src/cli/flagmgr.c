/*
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define LOGGER_PREFIX LOGGER_CUR_FILE

#include <lotto/base/flag.h>
#include <lotto/base/record_granularity.h>
#include <lotto/base/stable_address.h>
#include <lotto/base/topic.h>
#include <lotto/brokers/pubsub.h>
#include <lotto/brokers/statemgr.h>
#include <lotto/cli/flagmgr.h>
#include <lotto/cli/flags/prng.h>
#include <lotto/cli/subcmd.h>
#include <lotto/modules/enforce/state.h>
#include <lotto/modules/termination/state.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/now.h>
#include <lotto/sys/stdio.h>
#include <lotto/sys/stdlib.h>
#include <lotto/sys/string.h>
#include <lotto/util/once.h>

#define STR_CONVERTER_IS_PRESENT(conv) ((conv)._type != STR_CONVERTER_TYPE_NONE)

DECLARE_FLAG_HELP;

static void
_bits_str(str_converter_t *str_converter, uint64_t bits, char *dst)
{
    ASSERT(str_converter);
    switch (str_converter->_type) {
        case STR_CONVERTER_TYPE_BITS_GET:
            sys_strcpy(dst, str_converter->_bits_get.str(bits));
            break;
        case STR_CONVERTER_TYPE_BITS_PRINT:
            str_converter->_bits_print.str(bits, dst);
            break;
        default:
            ASSERT(0 && "unexpected string converter type");
            break;
    }
}

static uint64_t
_bits_from(str_converter_t *str_converter, const char *src)
{
    ASSERT(str_converter);
    switch (str_converter->_type) {
        case STR_CONVERTER_TYPE_BITS_GET:
            return str_converter->_bits_get.from(src);
        case STR_CONVERTER_TYPE_BITS_PRINT:
            return str_converter->_bits_print.from(src);
        default:
            ASSERT(0 && "unexpected string converter type");
            return 0;
    }
}

const char *
enabled_str(bool b)
{
    return b ? "enable" : "disable";
}

bool
enabled_from(const char *src)
{
    bool is_true  = sys_strcmp(src, "enable") == 0,
         is_false = sys_strcmp(src, "disable") == 0;
    ASSERT(is_true != is_false);
    return is_true;
}

#define BUFFER_LEN  1024
#define MAX_OPTIONS 1024

static flag_t _next_option = FLAG_NONE + 1;
static option_t _options[MAX_OPTIONS + 1];
static bool _sub_enabled = true;

static bool
_entry_has_short_opt(const option_t *e)
{
    return e->opt[0] != '\0';
}

static bool
_entry_has_long_opt(const option_t *e)
{
    return e->long_opt[0] != '\0';
}

const option_t *
flagmgr_options()
{
    return _options;
}

flag_t
new_flag(const char *name, const char *opt, const char *long_opt,
         const char *help, const char *desc, struct flag_val val,
         str_converter_t str_converter, flag_callback_f *callback)
{
    ASSERT(_sub_enabled);
    ASSERT(_next_option < MAX_OPTIONS);
    ASSERT(opt);
    ASSERT(long_opt);
    ASSERT(opt[0] != '\0' || long_opt[0] != '\0');
    ASSERT(help);
    ASSERT(desc);
    option_t entry = (option_t){.opt           = opt,
                                .long_opt      = long_opt,
                                .help          = help,
                                .desc          = desc,
                                .val           = val,
                                .callback      = callback,
                                .str_converter = str_converter,
                                .name          = name};
    for (flag_t f = FLAG_NONE + 1; f < _next_option; f++) {
        option_t *e = &_options[f];
        if ((_entry_has_short_opt(&entry) && sys_strcmp(opt, e->opt) != 0) ||
            (_entry_has_long_opt(&entry) &&
             sys_strcmp(long_opt, e->long_opt) != 0)) {
            continue;
        }
        entry.flag = f;
        ASSERT(sys_strcmp(entry.opt, e->opt) == 0);
        ASSERT(sys_strcmp(entry.long_opt, e->long_opt) == 0);
        ASSERT(sys_strcmp(entry.desc, e->desc) == 0);
        ASSERT(entry.callback == e->callback);
        ASSERT(entry.val._val._type == e->val._val._type);
        switch (entry.val._val._type) {
            case VALUE_TYPE_BOOL:
                ASSERT(is_on(entry.val._val) == is_on(e->val._val));
                break;
            case VALUE_TYPE_UINT64:
                ASSERT(as_uval(entry.val._val) == as_uval(e->val._val));
                break;
            case VALUE_TYPE_STRING:
                ASSERT(sys_strcmp(as_sval(entry.val._val),
                                  as_sval(e->val._val)) == 0);
                break;
            case VALUE_TYPE_ANY:
                ASSERT(as_any(entry.val._val) == as_any(e->val._val));
                break;
            default:
                ASSERT(0 && "unsupported value type");
                break;
        }
        ASSERT(sys_strcmp(entry.name, e->name) == 0);
        return f;
    }
    flag_t flag = entry.flag = _next_option++;
    _options[flag]           = entry;
    return flag;
}

void
flags_publish(const flags_t *flags)
{
    for (flag_t i = 0; i < _next_option; i++) {
        option_t *opt = _options + i;
        if (!opt->callback)
            continue;
        opt->callback(flags_get_value(flags, i), NULL);
    }
}


static flags_t *_default_flags;

static bool
_entry_has_argument(const option_t *e)
{
    return flag_type(e->val) != VALUE_TYPE_BOOL ||
           STR_CONVERTER_IS_PRESENT(e->str_converter);
}

static void
_init()
{
    ONCE();
    _default_flags = flags_alloc(_next_option);
    for (flag_t b = FLAG_NONE + 1; b < _next_option; b++) {
        flags_set(_default_flags, b, _options[b].val._val,
                  _options[b].val.force_no_default, true);
    }
}

flags_t *
flags_default()
{
    _init();
    return _default_flags;
}

flags_t *
flagmgr_flags_alloc()
{
    _sub_enabled = false;
    return flags_alloc(_next_option);
}

static char _flags_buffer[BUFFER_LEN];

static void
_flags_text(const flags_t *flags, const char *text[])
{
    int len     = sizeof(_flags_buffer);
    char *dst   = _flags_buffer;
    int written = 0;

    for (flag_t b = FLAG_NONE + 1; b < _next_option; b++) {
        if (flags_get(flags, b).force_no_default) {
            ASSERT(b == flag_seed() && "no help text defined for FLAG");
            text[b] = "current time in ns";
        }
        switch (flags_get_type(flags, b)) {
            case VALUE_TYPE_UINT64:
                text[b] = dst;
                if (STR_CONVERTER_IS_PRESENT(_options[b].str_converter)) {
                    _bits_str(&_options[b].str_converter,
                              flags_get_uval(flags, b), dst);
                    dst += sys_strlen(dst) + 1;
                } else {
                    written = 1 + sys_snprintf(dst, len, "%lu",
                                               flags_get_uval(flags, b));
                    dst += written;
                    len -= written;
                    ASSERT(len > 0);
                }
                break;
            case VALUE_TYPE_STRING:
                text[b] = flags_get_sval(flags, b);
                break;
            case VALUE_TYPE_BOOL:
                text[b] = STR_CONVERTER_IS_PRESENT(_options[b].str_converter) ?
                              enabled_str(flags_is_on(flags, b)) :
                          flags_is_on(flags, b) ? "on" :
                                                  "off";
                break;
            default:
                sys_fprintf(stdout, "%x\n", b);
                ASSERT(0 && "Unexpected flag type");
        }
    }
}

static int cur_longopt_val = 0;

struct option
_make_opt(char *opts, flag_t b)
{
    ASSERT(b != FLAG_NONE);
    bool has_short = _entry_has_short_opt(&_options[b]);
    bool has_arg   = _entry_has_argument(&_options[b]);
    if (has_short) {
        sys_strcat(opts, _options[b].opt);
        if (has_arg) {
            sys_strcat(opts, ":");
        }
    }
    return (struct option){.name    = _options[b].long_opt,
                           .has_arg = has_arg,
                           .flag    = &cur_longopt_val,
                           .val     = (int)b};
}

void
flags_opts(char *opts, bool runtime_sel, const flag_t sel[MAX_FLAGS],
           struct option long_opts[])
{
    /* before concatenating, we need to "clear" the string. */
    opts[0] = '\0';
    int i;
    for (i = 0; sel[i] != FLAG_SEL_SENTINEL; i++) {
        ASSERT(!runtime_sel || _options[sel[i]].callback == NULL);
        long_opts[i] = _make_opt(opts, sel[i]);
    }
    if (!runtime_sel) {
        return;
    }
    for (flag_t f = FLAG_NONE + 1; f < _next_option; f++) {
        if (_options[f].callback == NULL) {
            continue;
        }
        long_opts[i++] = _make_opt(opts, f);
    }
}

static void
_pad_block(size_t content_len, size_t len)
{
    for (size_t i = content_len; i < len; i++)
        sys_fprintf(stdout, " ");
}

void
_flag_help_print(flag_t b, size_t max_len, const char **default_values)
{
    const option_t *e = &_options[b];
    bool has_short    = _entry_has_short_opt(e);
    sys_fprintf(stdout, "    ");
    if (has_short) {
        sys_fprintf(stdout, "-%c, ", e->opt[0]);
    }
    sys_fprintf(stdout, "--%s %s", e->long_opt, e->help);
    _pad_block(sys_strlen(e->long_opt) + sys_strlen(e->help) +
                   ((size_t)4) * has_short + 3,
               max_len + 2);
    sys_fprintf(stdout, "%s", e->desc);
    if (default_values[b]) {
        sys_fprintf(stdout, " (default '%s')", default_values[b]);
    }
    sys_fprintf(stdout, "\n");
}

size_t
_flag_help_len(flag_t b)
{
    const option_t *e = &_options[b];
    return sys_strlen(e->long_opt) + sys_strlen(e->help) + 7;
}

void
flags_help(const flags_t *flags, bool runtime_sel, flag_t sel[MAX_FLAGS])
{
    const char **default_values = sys_calloc(sizeof(char *), _next_option);
    _flags_text(flags ? flags : flags_default(), default_values);

    flag_sel_add(sel, FLAG_HELP);

    size_t max_len = 0;
    for (int i = 0; sel[i] != FLAG_SEL_SENTINEL; i++) {
        size_t len = _flag_help_len(sel[i]);
        if (len > max_len)
            max_len = len;
    }
    if (runtime_sel) {
        for (flag_t f = FLAG_NONE + 1; f < _next_option; f++) {
            if (_options[f].callback == NULL) {
                continue;
            }
            size_t len = _flag_help_len(f);
            if (len > max_len)
                max_len = len;
        }
    }
    sys_fprintf(stdout, "Options:\n");
    for (int i = 0; sel[i] != FLAG_SEL_SENTINEL; i++) {
        _flag_help_print(sel[i], max_len, default_values);
    }
    if (runtime_sel) {
        for (flag_t f = FLAG_NONE + 1; f < _next_option; f++) {
            if (_options[f].callback == NULL) {
                continue;
            }
            _flag_help_print(f, max_len, default_values);
        }
    }
    sys_free(default_values);
}

static flag_t
_flags_with_opt(int64_t o)
{
    for (flag_t b = FLAG_NONE + 1; b < _next_option; b++)
        if (_options[b].opt[0] == o || b == o)
            return b;
    return FLAG_NONE;
}

void
enforce_no_default(flags_t *flags)
{
    const flags_t *default_flags = flags_default();
    for (flag_t b = FLAG_NONE + 1; b < _next_option; b++) {
        if (!(default_flags->values[b].force_no_default))
            continue;
        switch (flags_get_type(default_flags, b)) {
            case VALUE_TYPE_BOOL:
                if (flags_is_on(default_flags, b) != flags_is_on(flags, b))
                    continue;
                break;
            case VALUE_TYPE_UINT64:
                if (flags_get_uval(default_flags, b) !=
                    flags_get_uval(flags, b))
                    continue;
                break;
            case VALUE_TYPE_STRING:
                if (sys_strcmp(flags_get_sval(default_flags, b),
                               flags_get_sval(flags, b)) != 0)
                    continue;
                break;
            default:
                ASSERT(false && "Flag with unknown value type.");
                break;
        }

        ASSERT(b == flag_seed() &&
               "No action implemented for enforced non-default behavior");
        if (flags_get(flags, b).is_default) {
            flags_set_by_opt(flags, b, uval(now()));
        }
    }
    return;
}

int
flags_parse(flags_t *flags, args_t *args, bool runtime_sel,
            flag_t sel[MAX_FLAGS])
{
    flag_sel_add(sel, FLAG_HELP);
    char opts[BUFFER_LEN]    = {0};
    struct option *long_opts = sys_calloc(sizeof(struct option), _next_option);
    ;
    flags_opts(opts, runtime_sel, sel, long_opts);
    int c;
    int option_index = 0;

    /* reset extern opt index to allow multiple calls of getopt */
    optind = 0;

    /* overwrite defaults with options from args */
    while (cur_longopt_val = 0,
           ((c = getopt_long(args->argc, args->argv, opts, long_opts,
                             &option_index)) != -1)) {
        // ASSERT(c <= UINT8_MAX);
        flag_t fnum = _flags_with_opt(c);
        switch (c) {
            case 'h':
                return FLAGS_PARSE_HELP;
            case '?':
                return FLAGS_PARSE_ERROR;
            default:
                fnum = (unsigned int)cur_longopt_val ?: _flags_with_opt(c);
                if (fnum == FLAG_NONE) {
                    sys_fprintf(stdout, "unsupported flag 0%o ??\n", c);
                    continue;
                }
        }

        if (fnum == FLAG_HELP) {
            return FLAGS_PARSE_HELP;
        }

        switch (flags_get_type(flags, fnum)) {
            case VALUE_TYPE_UINT64:
                flags_set_by_opt(
                    flags, fnum,
                    uval(
                        STR_CONVERTER_IS_PRESENT(_options[fnum].str_converter) ?
                            _bits_from(&_options[fnum].str_converter, optarg) :
                            strtoul(optarg, NULL, 10)));
                break;
            case VALUE_TYPE_BOOL:
                flags_set_by_opt(flags, fnum,
                                 bval(STR_CONVERTER_IS_PRESENT(
                                          _options[fnum].str_converter) ?
                                          enabled_from(optarg) :
                                          !flags_is_on(flags, fnum)));
                break;
            case VALUE_TYPE_STRING:
                flags_set_by_opt(flags, fnum, sval(optarg));
                break;
            default:
                ASSERT(0);
        }
    }

    enforce_no_default(flags);

    args_shift(args, optind);
    return FLAGS_PARSE_OK;
}

/*******************************************************************************
 * printing
 ******************************************************************************/
void
flags_print(const flags_t *f)
{
    for (flag_t i = FLAG_NONE + 1; i < f->nflags; i++) {
        if (!_options[i].long_opt) {
            continue;
        }
        logger_infof("%-50s", _options[i].long_opt);
        const struct flag_val *flag_val = &f->values[i];
        const struct value *val         = &flag_val->_val;
        enum value_type type            = val->_type;
        switch (type) {
            case VALUE_TYPE_BOOL:
                logger_printf(
                    "%s", STR_CONVERTER_IS_PRESENT(_options[i].str_converter) ?
                              enabled_str(is_on(*val)) :
                          is_on(*val) ? "on" :
                                        "off");
                break;
            case VALUE_TYPE_UINT64:
                if (STR_CONVERTER_IS_PRESENT(_options[i].str_converter)) {
                    char temp[BUFFER_LEN];
                    _bits_str(&_options[i].str_converter, as_uval(*val), temp);
                    logger_printf("'%s'", temp);
                } else {
                    logger_printf("%lu", val->_uval);
                }
                break;
            case VALUE_TYPE_STRING:
                logger_printf("'%s'", val->_sval);
                break;
            default:
                ASSERT(0 && "unsupported value type");
        }
        logger_printf(" (%s)\n", flag_val->is_default ? "default" : "set");
    }
}
