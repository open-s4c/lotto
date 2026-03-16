/**
 * @file args.h
 * @brief Driver declarations for args.
 */
#ifndef LOTTO_DRIVER_ARGS_H
#define LOTTO_DRIVER_ARGS_H

#define ARGS(C, V)                                                             \
    (args_t)                                                                   \
    {                                                                          \
        .argc = (C), .argv = (V),                                              \
    }

typedef struct {
    int argc;
    char **argv;
    char *arg0;
} args_t;

void args_print(const args_t *args);
void args_shift(args_t *args, int n);

#endif
