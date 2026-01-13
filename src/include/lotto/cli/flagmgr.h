/*
 */
#ifndef LOTTO_FLAGMRG_H
#define LOTTO_FLAGMRG_H

#include <getopt.h>

#include <lotto/base/flags.h>
#include <lotto/cli/args.h>

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
        STR_CONVERTER_TYPE_BOOL, ._bool = {                                    \
            enabled_str,                                                       \
            enabled_from,                                                      \
            "enable|disable"                                                   \
        }                                                                      \
    }

const char *enabled_str(bool b);
bool enabled_from(const char *src);

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

#define _FLAGMGR_CALLBACK(name, CALLBACK)                                      \
    static void _flagmgr_callback_##name(struct value v, void *__)             \
    {                                                                          \
        (CALLBACK);                                                            \
    }

#define NEW_PRETTY_CALLBACK_FLAG(name, opt, long_opt, desc, val,               \
                                 str_converter, CALLBACK)                      \
    _FLAGMGR_CALLBACK(name, CALLBACK)                                          \
    static void LOTTO_CONSTRUCTOR _flagmgr_constructor_##name(void)            \
    {                                                                          \
        new_flag(#name, (opt), (long_opt), "", (desc), (val), (str_converter), \
                 _flagmgr_callback_##name);                                    \
    }

#define NEW_PUBLIC_PRETTY_CALLBACK_FLAG(var, opt, long_opt, desc, val,         \
                                        str_converter, CALLBACK)               \
    static flag_t FLAG_##var;                                                  \
    _FLAGMGR_CALLBACK(var, CALLBACK)                                           \
    static void LOTTO_CONSTRUCTOR _flagmgr_constructor_##var(void)             \
    {                                                                          \
        FLAG_##var = new_flag(#var, (opt), (long_opt), "", (desc), (val),      \
                              (str_converter), _flagmgr_callback_##var);       \
    }

#define NEW_CALLBACK_FLAG(name, opt, long_opt, help, desc, val, CALLBACK)      \
    _FLAGMGR_CALLBACK(name, CALLBACK)                                          \
    static void LOTTO_CONSTRUCTOR _flagmgr_constructor_##name(void)            \
    {                                                                          \
        new_flag(#name, (opt), (long_opt), (help), (desc), (val),              \
                 (str_converter_t){}, CONCAT(_flagmgr_callback_, name));       \
    }

#define NEW_PUBLIC_CALLBACK_FLAG(var, opt, long_opt, help, desc, val,          \
                                 CALLBACK)                                     \
    static flag_t FLAG_##var;                                                  \
    _FLAGMGR_CALLBACK(var, CALLBACK)                                           \
    static void LOTTO_CONSTRUCTOR _flagmgr_constructor_##var(void)             \
    {                                                                          \
        FLAG_##var = new_flag(#var, (opt), (long_opt), (help), (desc), (val),  \
                              (str_converter_t){}, _flagmgr_callback_##var);   \
    }

#define NEW_PUBLIC_PRETTY_COMMAND_FLAG(var, opt, long_opt, help, desc, val,    \
                                       str_converter)                          \
    static void LOTTO_CONSTRUCTOR _flagmgr_constructor_##var(void)             \
    {                                                                          \
        var = new_flag(#var, (opt), (long_opt), (help), (desc), (val),         \
                       (str_converter), NULL);                                 \
    }

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
    DECLARE_COMMAND_FLAG(VERBOSE, "v", "verbose", "", "verbose", flag_off())

#define DECLARE_FLAG_CREP                                                      \
    DECLARE_COMMAND_FLAG(CREP, "", "crep", "", "enable crep", flag_off())

#define DECLARE_FLAG_REPLAY_GOAL                                               \
    DECLARE_COMMAND_FLAG(REPLAY_GOAL, "g", "goal", "INT", "replay goal",       \
                         flag_uval(MAX_ROUNDS))

#define DECLARE_FLAG_LOGGER_FILE                                                  \
    DECLARE_COMMAND_FLAG(LOGGER_FILE, "", "log", "FILE", "output log FILE",       \
                         flag_sval("stderr"))

#define DECLARE_FLAG_TEMPORARY_DIRECTORY                                       \
    DECLARE_COMMAND_FLAG(TEMPORARY_DIRECTORY, "", "temporary-directory",       \
                         "DIR", "temporary directory to write Lotto files",    \
                         flag_sval(get_default_temporary_directory()))

#define DECLARE_FLAG_NO_PRELOAD                                                \
    DECLARE_COMMAND_FLAG(                                                      \
        NO_PRELOAD, "", "no-preload", "",                                      \
        "does not preload lotto shared library, used for qlotto", flag_off())

#define DECLARE_FLAG_VERSION                                                   \
    DECLARE_COMMAND_FLAG(VERSION, "", "version", "", "", flag_off())

#define DECLARE_FLAG_LOGGER_BLOCK                                              \
    DECLARE_COMMAND_FLAG(                                                      \
        LOGGER_BLOCK, "", "logger-block", "NAME1|NAME2|...",                   \
        "set file name without suffix to disable logger message",              \
        flag_sval(""))

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

#define DECLARE_FLAG_FLOTTO                                                    \
    DECLARE_COMMAND_FLAG(FLOTTO, "f", "flotto", "",                            \
                         "fuzzer without replay possibility", flag_off())

/**
 * CLI parsing and printing
 */

#define FLAGS_PARSE_OK    0
#define FLAGS_PARSE_ERROR 1
#define FLAGS_PARSE_HELP  -1

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
