/*
 */
#include <stdbool.h>
#include <string.h>

#define LOGGER_PREFIX LOGGER_CUR_FILE
#define LOGGER_BLOCK  LOGGER_CUR_BLOCK
#include <lotto/base/callrec.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger_block.h>
#include <lotto/sys/stdio.h>
#include <lotto/sys/stdlib.h>
#include <lotto/sys/string.h>
#include <sys/stat.h>
#include <sys/types.h>

static const int CANARY = 0x3F3F3F;

callrec_t
callrec_create(const char *fn)
{
    ASSERT(fn && sys_strlen(fn) > 0);

    FILE *fp = NULL;
    struct stat sbuf;
    if (stat(fn, &sbuf) == 0)
        fp = sys_fopen(fn, "r+");
    else
        fp = sys_fopen(fn, "w+");

    if (fp == NULL)
        logger_fatalf("could not open file '%s'", fn);

    callrec_t r = {
        .fp        = fp,
        .replaying = true,
    };
    strcpy(r.filename, fn);
    return r;
}

int
callrec_close(callrec_t *r)
{
    int ret = 0;
    if (r->fp != NULL)
        ret = sys_fclose(r->fp);
    r->fp = NULL;
    return ret == 0 ? CALLREC_OK : CALLREC_ERROR;
}

int
callrec_record(callrec_t *r, const callfun_t *fun)
{
    if (r->replaying) {
        if (feof(r->fp))
            clearerr(r->fp);
        r->replaying = false;
    }
    callfun_t f = *fun;
    f.canary    = CANARY;
    if (sys_fwrite(&f, sizeof(callfun_t), 1, r->fp) < 1) {
        return CALLREC_RECORD_ERROR;
    }

    if (f.retval) {
        if (sys_fwrite(f.retval, 1, f.retsize, r->fp) < f.retsize) {
            return CALLREC_RECORD_ERROR;
        }
    }

    for (size_t i = 0; fun->params[i].data && i < MAX_PARAMS; i++) {
        const callparam_t p = fun->params[i];
        if (sys_fwrite(p.data, 1, p.size, r->fp) < p.size) {
            return CALLREC_RECORD_ERROR;
        }
    }

    fflush(r->fp);
    return CALLREC_OK;
}

int
callrec_replay(const callrec_t *r, callfun_t *fun)
{
    ASSERT(r->replaying);
    callfun_t lfun;
    if (sys_fread(&lfun, sizeof(callfun_t), 1, r->fp) != 1) {
        return CALLREC_REPLAY_ERROR;
    }

    ASSERT(lfun.canary == CANARY);
    ASSERT(lfun.retsize == fun->retsize);
    if (fun->retsize) {
        if (fun->retval == NULL)
            fun->retval = sys_malloc(fun->retsize);
        if (sys_fread((void *)fun->retval, 1, fun->retsize, r->fp) < 1) {
            return CALLREC_REPLAY_ERROR;
        }
    }

    for (size_t i = 0; fun->params[i].data && i < MAX_PARAMS; i++) {
        callparam_t *param = &fun->params[i];
        ASSERT(param->size > 0);
        if (param->data == NULL)
            param->data = sys_malloc(param->size);
        if (sys_fread((void *)param->data, 1, param->size, r->fp) <
            param->size) {
            return CALLREC_REPLAY_ERROR;
        }
    }

    return CALLREC_OK;
}
