/**
 * @file flagmgr.h
 * @brief Driver declarations for flagmgr.
 */
#ifndef LOTTO_FLAGMRG_H
#define LOTTO_FLAGMRG_H

#include <getopt.h>

#include <lotto/base/flags.h>
#include <lotto/driver/args.h>
#include <lotto/engine/pubsub.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger.h>
#include <lotto/sys/string.h>
#include <lotto/util/macros.h>

/**
 * Pretty printers
 */

typedef void(bits_str_print)(uint64_t bits, char *dst);
typedef void(bits_str_help)(char *dst);
typedef const char *(bits_str_get)(uint64_t bits);
typedef uint64_t(bits_from)(const char *src);
typedef const char *(bool_str)(bool b);
typedef bool(bool_from)(const char *src);

enum str_converter_type {
    STR_CONVERTER_TYPE_NONE,
    STR_CONVERTER_TYPE_BITS_GET,
    STR_CONVERTER_TYPE_BITS_PRINT,
    STR_CONVERTER_TYPE_BOOL,
};

typedef struct str_converter_s {
    enum str_converter_type _type;
    union {
        struct {
            bits_str_get *str;
            bits_from *from;
            size_t size;
            bits_str_help *help;
        } _bits_get;
        struct {
            bits_str_print *str;
            bits_from *from;
            size_t size;
            bits_str_help *help;
        } _bits_print;
        struct {
            bool_str *str;
            bool_from *from;
            const char *help;
        } _bool;
    };
} str_converter_t;

#define STR_CONVERTER_GET(str, from, size, help)                               \
    (str_converter_t)                                                          \
    {                                                                          \
        STR_CONVERTER_TYPE_BITS_GET, ._bits_get = {                            \
            (str),                                                             \
            (from),                                                            \
            (size),                                                            \
            (help)                                                             \
        }                                                                      \
    }

#define STR_CONVERTER_PRINT(str, from, size, help)                             \
    (str_converter_t)                                                          \
    {                                                                          \
        STR_CONVERTER_TYPE_BITS_PRINT, ._bits_print = {                        \
            (str),                                                             \
            (from),                                                            \
            (size),                                                            \
            (help)                                                             \
        }                                                                      \
    }

#define STR_CONVERTER_BOOL                                                     \
    (str_converter_t)                                                          \
    {                                                                          \
        STR_CONVERTER_TYPE_BOOL,                                               \
            ._bool = {enabled_str, enabled_from, "enable|disable"}             \
    }

static inline const char *
enabled_str(bool b)
{
    return b ? "enable" : "disable";
}

static inline bool
enabled_from(const char *src)
{
    bool is_true = sys_strcmp(src, "enable") == 0 ||
                   sys_strcmp(src, "on") == 0 || sys_strcmp(src, "true") == 0 ||
                   sys_strcmp(src, "1") == 0;
    bool is_false = sys_strcmp(src, "disable") == 0 ||
                    sys_strcmp(src, "off") == 0 ||
                    sys_strcmp(src, "false") == 0 || sys_strcmp(src, "0") == 0;
    ASSERT(is_true != is_false);
    return is_true;
}

/**
 * Flag registration
 */

#define MAX_FLAGS 128

typedef void(flag_callback_f)(struct value v, void *arg);

struct entry {
    const char *opt;
    const char *long_opt;
    const char *help;
    const char *desc;
    struct flag_val val;
    str_converter_t str_converter;
    flag_callback_f *callback;
};

flag_t new_flag(const char *name, const char *opt, const char *long_opt,
                const char *help, const char *desc, struct flag_val val,
                str_converter_t str_converter, flag_callback_f *callback);
void flags_publish(const flags_t *flags);
const char *get_default_temporary_directory();
flags_t *flags_default();
flag_t flag_input();
flag_t flag_output();
flag_t flag_rounds();
flag_t flag_verbose();
flag_t flag_replay_goal();
flag_t flag_temporary_directory();
flag_t flag_no_preload();
flag_t flag_before_run();
flag_t flag_after_run();
flag_t flag_logger_file();

static inline uint64_t
flag_verbose_count(const flags_t *flags)
{
    return flags_get_uval(flags, flag_verbose());
}

static inline bool
flag_verbose_enabled(const flags_t *flags)
{
    return flag_verbose_count(flags) > 0;
}

static inline enum logger_level
flag_verbose_logger_level(const flags_t *flags)
{
    uint64_t verbose = flag_verbose_count(flags);
    if (verbose >= 2) {
        return LOGGER_DEBUG;
    }
    if (verbose >= 1) {
        return LOGGER_INFO;
    }
    return LOGGER_ERROR;
}

static inline const char *
flag_verbose_logger_level_str(const flags_t *flags)
{
    switch (flag_verbose_logger_level(flags)) {
        case LOGGER_DEBUG:
            return "debug";
        case LOGGER_INFO:
            return "info";
        case LOGGER_WARN:
            return "warn";
        case LOGGER_ERROR:
            return "error";
        default:
            ASSERT(false && "unexpected logger level");
            return "error";
    }
}

#define _FLAGMGR_CALLBACK(name, ...)                                           \
    static void _flagmgr_callback_##name(struct value v, void *__)             \
    {                                                                          \
        __VA_ARGS__;                                                           \
    }

#define ON_DRIVER_REGISTER_FLAGS(...)                                          \
    LOTTO_SUBSCRIBE_CONTROL(EVENT_DRIVER__REGISTER_FLAGS, __VA_ARGS__)

#define ON_DRIVER_REGISTER_COMMANDS(...)                                       \
    LOTTO_SUBSCRIBE_CONTROL(EVENT_DRIVER__REGISTER_COMMANDS, __VA_ARGS__)

#define _FLAGMGR_SUBSCRIBE(...) ON_DRIVER_REGISTER_FLAGS(__VA_ARGS__)

#ifdef LOTTO_MODULE_NAME
    #define LOTTO_MODULE_FLAG(name) LOTTO_MODULE_NAME "-" name
#endif

#define NEW_PRETTY_CALLBACK_FLAG(name, opt, long_opt, desc, val,               \
                                 str_converter, ...)                           \
    _FLAGMGR_CALLBACK(name, __VA_ARGS__)                                       \
    _FLAGMGR_SUBSCRIBE({                                                       \
        new_flag(#name, (opt), (long_opt), "", (desc), (val), (str_converter), \
                 _flagmgr_callback_##name);                                    \
    })

#define NEW_PUBLIC_PRETTY_CALLBACK_FLAG(var, opt, long_opt, desc, val,         \
                                        str_converter, ...)                    \
    static flag_t FLAG_##var;                                                  \
    _FLAGMGR_CALLBACK(var, __VA_ARGS__)                                        \
    _FLAGMGR_SUBSCRIBE({                                                       \
        FLAG_##var = new_flag(#var, (opt), (long_opt), "", (desc), (val),      \
                              (str_converter), _flagmgr_callback_##var);       \
    })

#define NEW_CALLBACK_FLAG(name, opt, long_opt, help, desc, val, ...)           \
    _FLAGMGR_CALLBACK(name, __VA_ARGS__)                                       \
    _FLAGMGR_SUBSCRIBE({                                                       \
        new_flag(#name, (opt), (long_opt), (help), (desc), (val),              \
                 (str_converter_t){}, CONCAT(_flagmgr_callback_, name));       \
    })

#define NEW_PUBLIC_CALLBACK_FLAG(var, opt, long_opt, help, desc, val, ...)     \
    static flag_t FLAG_##var;                                                  \
    _FLAGMGR_CALLBACK(var, __VA_ARGS__)                                        \
    _FLAGMGR_SUBSCRIBE({                                                       \
        FLAG_##var = new_flag(#var, (opt), (long_opt), (help), (desc), (val),  \
                              (str_converter_t){}, _flagmgr_callback_##var);   \
    })

#define NEW_PUBLIC_PRETTY_COMMAND_FLAG(var, opt, long_opt, help, desc, val,    \
                                       str_converter)                          \
    _FLAGMGR_SUBSCRIBE({                                                       \
        var = new_flag(#var, (opt), (long_opt), (help), (desc), (val),         \
                       (str_converter), NULL);                                 \
    })

#define NEW_PUBLIC_COMMAND_FLAG(var, opt, long_opt, help, desc, val)           \
    static flag_t FLAG_##var;                                                  \
    NEW_PUBLIC_PRETTY_COMMAND_FLAG(FLAG_##var, (opt), (long_opt), (help),      \
                                   (desc), (val), (str_converter_t){})

#define DECLARE_COMMAND_FLAG(name, opt, long_opt, help, desc, val)             \
    NEW_PUBLIC_COMMAND_FLAG(name, (opt), (long_opt), (help), (desc), (val))

#define FLAG_GETTER(name, NAME)                                                \
    flag_t flag_##name()                                                       \
    {                                                                          \
        ASSERT(FLAG_##NAME);                                                   \
        return FLAG_##NAME;                                                    \
    }

/**
 * Common CLI flags
 */

#define DECLARE_FLAG_INPUT                                                     \
    DECLARE_COMMAND_FLAG(INPUT, "i", "input-trace", "FILE",                    \
                         "input trace FILE", flag_sval("replay.trace"))

#define DECLARE_FLAG_OUTPUT                                                    \
    DECLARE_COMMAND_FLAG(OUTPUT, "o", "output-trace", "FILE",                  \
                         "output trace FILE", flag_sval("replay.trace"))

#define MAX_ROUNDS (~0UL)

#define DECLARE_FLAG_ROUNDS                                                    \
    DECLARE_COMMAND_FLAG(ROUNDS, "r", "rounds", "INT",                         \
                         "maximum number of rounds", flag_uval(MAX_ROUNDS))

#define DECLARE_FLAG_VERBOSE                                                   \
    DECLARE_COMMAND_FLAG(VERBOSE, "v", "verbose", "",                          \
                         "increase verbosity (repeat for more detail)",        \
                         flag_uval(0))

#define DECLARE_FLAG_REPLAY_GOAL                                               \
    DECLARE_COMMAND_FLAG(REPLAY_GOAL, "g", "goal", "INT", "replay goal",       \
                         flag_uval(MAX_ROUNDS))

#define DECLARE_FLAG_LOGGER_FILE                                               \
    DECLARE_COMMAND_FLAG(LOGGER_FILE, "", "log", "FILE", "output log FILE",    \
                         flag_sval("stderr"))

#define DECLARE_FLAG_TEMPORARY_DIRECTORY                                       \
    DECLARE_COMMAND_FLAG(TEMPORARY_DIRECTORY, "t", "temporary-directory",      \
                         "DIR", "temporary directory to write Lotto files",    \
                         flag_sval(get_default_temporary_directory()))

#define DECLARE_FLAG_NO_PRELOAD                                                \
    DECLARE_COMMAND_FLAG(                                                      \
        NO_PRELOAD, "", "no-preload", "",                                      \
        "does not preload lotto shared library, used for qlotto", flag_off())

#define DECLARE_FLAG_VERSION                                                   \
    DECLARE_COMMAND_FLAG(VERSION, "", "version", "", "", flag_off())

#define DECLARE_FLAG_BEFORE_RUN                                                \
    DECLARE_COMMAND_FLAG(BEFORE_RUN, "", "before-run", "CMD",                  \
                         "action to be executed before each run",              \
                         flag_sval(""))

#define DECLARE_FLAG_AFTER_RUN                                                 \
    DECLARE_COMMAND_FLAG(AFTER_RUN, "", "after-run", "CMD",                    \
                         "action to be executed after each run",               \
                         flag_sval(""))

#define DECLARE_FLAG_HELP                                                      \
    DECLARE_COMMAND_FLAG(HELP, "h", "help", "", "help message for flags",      \
                         flag_off())

#define DECLARE_FLAG_LIST_FLAGS                                                \
    DECLARE_COMMAND_FLAG(LIST_FLAGS, "", "list-flags", "",                     \
                         "list available flags", flag_off())

/**
 * CLI parsing and printing
 */

#define FLAGS_PARSE_OK         0
#define FLAGS_PARSE_ERROR      1
#define FLAGS_PARSE_HELP       -1
#define FLAGS_PARSE_LIST_FLAGS -2

void flags_print(const flags_t *flags);

/**
 * Initializes flags parsing args with selection sel of flags.
 *
 * @return Either FLAGS_PARSE_OK, FLAGS_PARSE_ERROR or FLAGS_PARSE_HELP
 */
int flags_parse(flags_t *flags, args_t *args, bool runtime_sel,
                flag_t sel[MAX_FLAGS]);
void flags_opts(char *opts, bool runtime_sel, const flag_t sel[MAX_FLAGS],
                struct option long_opts[]);
void flags_help(const flags_t *flags, bool runtime_sel, flag_t sel[MAX_FLAGS]);
void flags_command_help(const flags_t *flags, flag_t sel[MAX_FLAGS]);
void flags_runtime_help(const flags_t *flags, bool runtime_sel);
void flags_list(bool runtime_sel, flag_t sel[MAX_FLAGS]);
flags_t *flagmgr_flags_alloc();

typedef struct {
    flag_t flag;
    const char *name;
    const char *opt;
    const char *long_opt;
    const char *help;
    const char *desc;
    struct flag_val val;
    str_converter_t str_converter;
    flag_callback_f *callback;
} option_t;

const option_t *flagmgr_options();

#endif
