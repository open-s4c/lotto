/*
 */
#define LOG_PREFIX LOG_CUR_FILE
#define LOG_BLOCK  LOG_CUR_BLOCK
#include <lotto/brokers/statemgr.h>
#include <lotto/cli/exec_info.h>
#include <lotto/sys/logger_block.h>
#include <lotto/sys/stdio.h>
#include <lotto/sys/stdlib.h>
#include <lotto/sys/string.h>

static const char *_envvars[REPLAYED_ENVVARS] = {"LD_PRELOAD"};

/*******************************************************************************
 * marshaling interface
 ******************************************************************************/

#define MARSHABLE                                                              \
    (marshable_t)                                                              \
    {                                                                          \
        .marshal = _marshal, .unmarshal = _unmarshal, .size = _size,           \
        .print = _print                                                        \
    }

static size_t _size(const marshable_t *m);
static void *_marshal(const marshable_t *m, void *buf);
static const void *_unmarshal(marshable_t *m, const void *buf);
static void _print(const marshable_t *m);

/*******************************************************************************
 * state
 ******************************************************************************/

static exec_info_t _exec_info;

REGISTER_STATE(START, _exec_info, {
    _exec_info = (exec_info_t){.m = MARSHABLE};
    ASSERT(REPLAYED_ENVVARS == 0 || _envvars[REPLAYED_ENVVARS - 1] != NULL);
})

/*******************************************************************************
 * utils
 ******************************************************************************/
static void
_check_hash()
{
    bool hash_warning = true;
    if (_exec_info.hash_actual == 0) {
        sys_fprintf(
            stderr,
            "Warning: cannot compute hash of currently used lotto binary.");
    } else if (_exec_info.hash_replayed == 0) {
        sys_fprintf(
            stderr,
            "Warning: trace does not contain hash of lotto binary used for "
            "recording.");
    } else if (_exec_info.hash_actual != _exec_info.hash_replayed) {
        sys_fprintf(stderr,
                    "Warning: hashes of lotto binaries (current (0x%08lx) and "
                    "the one used "
                    "for recording 0x%08lx) differ.",
                    _exec_info.hash_actual, _exec_info.hash_replayed);
    } else {
        hash_warning = false;
    }
    if (hash_warning) {
        sys_fprintf(stderr,
                    "\nThis may affect the predictability of replay!\n\n");
    }
}

/*******************************************************************************
 * public interface
 ******************************************************************************/
exec_info_t *
get_exec_info()
{
    return &_exec_info;
}

void
exec_info_store_envvars()
{
    for (size_t i = 0; i < REPLAYED_ENVVARS; i++) {
        _exec_info.envvars[i] = getenv(_envvars[i]);
        ASSERT(_exec_info.envvars[i] != NULL);
    }
}

#define REPLAY_UNDEFINED 0
#define REPLAY_EMPTY     1
#define REPLAY_NONEMPTY  2

bool
exec_info_replay_envvars()
{
    uint8_t replay_state = REPLAY_UNDEFINED;
    for (size_t i = 0; i < REPLAYED_ENVVARS; i++) {
        if (_exec_info.envvars[i] == NULL) {
            ASSERT(replay_state == REPLAY_UNDEFINED ||
                   replay_state == REPLAY_EMPTY);
            replay_state = REPLAY_EMPTY;
            continue;
        }
        ASSERT(replay_state == REPLAY_UNDEFINED ||
               replay_state == REPLAY_NONEMPTY);
        replay_state = REPLAY_NONEMPTY;
        sys_setenv(_envvars[i], _exec_info.envvars[i], true);
    }
    return replay_state == REPLAY_NONEMPTY;
}

/*******************************************************************************
 * marshaling implementation
 ******************************************************************************/

static size_t
_size(const marshable_t *m)
{
    ASSERT(m);
    exec_info_t *ei = (exec_info_t *)m;
    size_t total    = sizeof(exec_info_t) - offsetof(exec_info_t, payload);
    args_t *args    = &ei->args;
    total += sys_strlen(args->arg0) + 1;
    for (int i = 0; i < args->argc; i++) {
        total += sys_strlen(args->argv[i]) + 1;
    }
    for (size_t i = 0; i < REPLAYED_ENVVARS; i++) {
        ASSERT(_exec_info.envvars[i] != NULL);
        total += sys_strlen(_exec_info.envvars[i]) + 1;
    }
    return total;
}

static void *
_marshal(const marshable_t *m, void *buf)
{
    ASSERT(m);
    ASSERT(buf);

    exec_info_t *ei   = (exec_info_t *)m;
    ei->hash_replayed = ei->hash_actual;
    size_t off        = offsetof(exec_info_t, payload);
    char *b           = (char *)buf;
    sys_memcpy(b, ei->payload, sizeof(exec_info_t) - off);
    b += sizeof(exec_info_t) - off;
    args_t *args = &ei->args;
    sys_strcpy(b, args->arg0);
    b += sys_strlen(args->arg0) + 1;
    for (int i = 0; i < args->argc; i++) {
        sys_strcpy(b, args->argv[i]);
        b += sys_strlen(args->argv[i]) + 1;
    }
    for (size_t i = 0; i < REPLAYED_ENVVARS; i++) {
        ASSERT(_exec_info.envvars[i] != NULL);
        sys_strcpy(b, _exec_info.envvars[i]);
        b += sys_strlen(_exec_info.envvars[i]) + 1;
    }
    return b;
}

static const void *
_unmarshal(marshable_t *m, const void *buf)
{
    ASSERT(m);
    ASSERT(buf);

    const char *b   = (const char *)buf;
    exec_info_t *ei = (exec_info_t *)m;
    size_t off      = offsetof(exec_info_t, payload);
    sys_memcpy(ei->payload, b,
               sizeof(exec_info_t) - offsetof(exec_info_t, payload));
    _check_hash();
    b += sizeof(exec_info_t) - off;
    args_t *args = &ei->args;
    size_t len   = sys_strlen(b) + 1;
    args->arg0   = sys_malloc(len);
    sys_strcpy(args->arg0, b);
    b += len;
    args->argv = sys_malloc((args->argc + 1) * sizeof(char *));
    for (int i = 0; i < args->argc; i++) {
        len           = sys_strlen(b) + 1;
        args->argv[i] = sys_malloc(len);
        sys_strcpy(args->argv[i], b);
        b += len;
    }
    args->argv[args->argc] = NULL;
    for (size_t i = 0; i < REPLAYED_ENVVARS; i++) {
        size_t len            = sys_strlen(b) + 1;
        _exec_info.envvars[i] = sys_malloc(len);
        sys_strcpy(_exec_info.envvars[i], b);
        b += sys_strlen(_exec_info.envvars[i]) + 1;
    }
    return b;
}

static void
_print(const marshable_t *m)
{
    ASSERT(m);
    exec_info_t *ei = (exec_info_t *)m;
    logger_infof("hash = 0x%08lx\n", ei->hash_replayed);
    logger_infof("args =");
    args_t *args = &ei->args;
    for (int i = 0; i < args->argc; i++) {
        logger_printf(" %s", args->argv[i]);
    }
    logger_println();
    logger_infof("envvars:\n");
    for (size_t i = 0; i < REPLAYED_ENVVARS; i++) {
        logger_infof("  %s=%s\n", _envvars[i], ei->envvars[i]);
    }
}
