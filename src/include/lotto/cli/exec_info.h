#ifndef LOTTO_CLI_EXEC_INFO_H
#define LOTTO_CLI_EXEC_INFO_H
#include <stdbool.h>
#include <stdint.h>

#include <lotto/base/marshable.h>
#include <lotto/cli/args.h>

#define REPLAYED_ENVVARS 1

typedef struct exec_info_s {
    marshable_t m;
    uint64_t hash_actual;
    char payload[0];
    uint64_t hash_replayed;
    args_t args;
    char *envvars[REPLAYED_ENVVARS];
} exec_info_t;

exec_info_t *get_exec_info();
void exec_info_store_envvars();
bool exec_info_replay_envvars();

#endif
