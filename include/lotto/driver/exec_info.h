/**
 * @file exec_info.h
 * @brief Driver declarations for exec info.
 */
#ifndef LOTTO_DRIVER_EXEC_INFO_H
#define LOTTO_DRIVER_EXEC_INFO_H

#include <stdbool.h>
#include <stdint.h>

#include <lotto/base/marshable.h>
#include <lotto/driver/args.h>

/** Number of envvars persisted in exec-info replay payloads. */
#define REPLAYED_ENVVARS 2

/** Driver execution metadata shared across command stages. */
typedef struct exec_info_s {
    /** Marshaling metadata for trace/config persistence. */
    marshable_t m;
    /** Hash of the currently running `lotto` binary. */
    uint64_t hash_actual;
    /** Flexible payload area for marshaled extensions. */
    char payload[0];
    /** Hash recovered from replay input (if any). */
    uint64_t hash_replayed;
    /** Parsed command arguments for the active subcommand. */
    args_t args;
    /** Captured envvars to replay when continuing executions. */
    char *envvars[REPLAYED_ENVVARS];
} exec_info_t;

/** Access singleton execution metadata. */
exec_info_t *get_exec_info();
/** Capture replay-sensitive envvars into exec-info state. */
void exec_info_store_envvars();
/** Restore captured envvars into process environment. */
bool exec_info_replay_envvars(int verbose);

#endif
