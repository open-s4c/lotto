/*
 */
#ifndef LOTTO_CALLREC_H
#define LOTTO_CALLREC_H

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>

#include <lotto/base/clk.h>
#include <lotto/base/task_id.h>

#define CALLREC_OK           0
#define CALLREC_ERROR        -1
#define CALLREC_REPLAY_ERROR -1
#define CALLREC_RECORD_ERROR -2

typedef struct callrec {
    char filename[PATH_MAX];
    bool replaying;
    FILE *fp;
} callrec_t;

typedef struct callparam {
    void *data;
    size_t size;
} callparam_t;

#define MAX_PARAMS 32
#define MAX_NAME   64
typedef struct callfun {
    int canary;
    char name[MAX_NAME + 1];
    void *retval;
    size_t retsize;
    clk_t clk;

    // NULL-terminated array
    callparam_t params[MAX_PARAMS + 1];
} callfun_t;

callrec_t callrec_create(const char *filename);

int callrec_close(callrec_t *r);
int callrec_record(callrec_t *r, const callfun_t *fun);
int callrec_replay(const callrec_t *r, callfun_t *fun);

#endif
