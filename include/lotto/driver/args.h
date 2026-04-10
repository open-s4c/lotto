/**
 * @file args.h
 * @brief Driver declarations for args.
 */
#ifndef LOTTO_DRIVER_ARGS_H
#define LOTTO_DRIVER_ARGS_H

/** Build an `args_t` literal from argc/argv values. */
#define ARGS(C, V)                                                             \
    (args_t)                                                                   \
    {                                                                          \
        .argc = (C), .argv = (V),                                              \
    }

/** Parsed command arguments handled by driver subcommands. */
typedef struct {
    /** Number of entries in `argv`. */
    int argc;
    /** Argument vector for the target command (NULL-terminated when used for
     * exec). */
    char **argv;
    /** Original executable path used to launch `lotto`. */
    char *arg0;
} args_t;

/** Print a human-readable command line representation. */
void args_print(const args_t *args);
/** Drop the first `n` entries from `args`. */
void args_shift(args_t *args, int n);

#endif
