/*
 */
#include <glob.h>
#include <limits.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#define LOG_PREFIX LOG_CUR_FILE
#define LOG_BLOCK  LOG_CUR_BLOCK
#include <crep.h>

#include <lotto/base/callrec.h>
#include <lotto/base/task_id.h>
#include <lotto/unsafe/_sys.h>
#include <vsync/atomic/core.h>
#include <vsync/atomic/dispatch.h>
#include <vsync/queue/bounded_mpmc.h>

#define MAX_THREADS 1024
bounded_mpmc_t pt_threads_queue;
pthread_t *pt_threads[MAX_THREADS];

static pthread_key_t key;
static pthread_once_t once_control = PTHREAD_ONCE_INIT;

struct crep {
    task_id id;
    bool replaying;
    bool disabled;
    callrec_t rec;
};

static vatomic32_t _count;
static void _crep_init(void);
static void _crep_fini(void);
static void _crep_dtor(void *arg);

static void
once_init_key(void)
{
    int ret = 0;
    ret     = pthread_key_create(&key, _crep_dtor);
    ASSERT((ret == 0) && "pthread_key_create failed");
}

static struct crep *
crep_get(void)
{
    struct crep *c = NULL;
    int ret        = 0;

    pthread_once(&once_control, once_init_key);

    if ((c = pthread_getspecific(key)) == NULL) {
        c = rl_malloc(sizeof(struct crep));
        ASSERT((c != NULL) && "rl_malloc failed");
        rl_memset(c, 0, sizeof(struct crep));

        ret = pthread_setspecific(key, (void *)c);
        ASSERT((ret == 0) && "pthread_setspecific failed");
    }
    return c;
}

void
crep_disable_()
{
    struct crep *c = crep_get();
    ASSERT(!c->disabled);
    c->disabled = true;
}

void
crep_enable_()
{
    struct crep *c = crep_get();
    ASSERT(c->disabled);
    c->disabled = false;
}

bool
crep_replay(callfun_t *fun)
{
    struct crep *c = crep_get();
    ;
    if (c->disabled)
        return false;

    _crep_init();

    if (!c->replaying)
        return false;

    int ok = callrec_replay(&c->rec, fun);
    if (ok == CALLREC_OK)
        return true;

    ASSERT(ok == CALLREC_REPLAY_ERROR);
    c->replaying = false;

    return false;
}

void
crep_record(const callfun_t *fun)
{
    struct crep *c = crep_get();
    if (c->disabled)
        return;

    _crep_init();

    if (c->replaying)
        c->replaying = false;

    int ok = callrec_record(&c->rec, fun);
    ASSERT(ok == CALLREC_OK);
}

static void
_crep_init()
{
    struct crep *c = crep_get();
    int ret        = 0;

    if (c->id != NO_TASK) {
        return;
    }
    c->id = vatomic_inc_get(&_count);
    char filename[PATH_MAX];
    rl_snprintf(filename, PATH_MAX, "task-%lu.crep", c->id);
    c->rec       = callrec_create(filename);
    c->replaying = true;

    if (c->id == 1) {
        ret = atexit(_crep_fini);
        ASSERT((ret == 0) && "failed to set exit function");
        bounded_mpmc_init(&pt_threads_queue, (void **)pt_threads, MAX_THREADS);
    } else {
        if (QUEUE_BOUNDED_OK != bounded_mpmc_enq(&pt_threads_queue, c)) {
            log_fatalf("unexpected enqueue error");
        }
    }
}

static void
_crep_fini(void)
{
    struct crep *_crep = crep_get();

    if (_crep->id == NO_TASK) {
        return;
    }
    // we are main thread, close callrec instances of all other threads
    if (_crep->id == 1) {
        crep_t *remote_crep;
        while (QUEUE_BOUNDED_OK ==
               bounded_mpmc_deq(&pt_threads_queue, (void **)&remote_crep)) {
            callrec_close(&remote_crep->rec);
        }
    }
    callrec_close(&_crep->rec);
}

static void
_crep_dtor(void *arg)
{
    _crep_fini();
    rl_free(arg);
}

#ifdef CREP_STANDALONE
void *
crep_warn_call(const char *func)
{
    struct crep *c = crep_get();

    _crep_init();
    log_warnf("[%lu] warn call '%s'\n", c->id, func);

    /* search for the real function and return its pointer */
    void *foo = real_func(func, NULL);
    if (foo == NULL)
        log_fatalf("could not find function '%s'\n", func);
    return foo;
}

void *
crep_abort_call(const char *func)
{
    struct crep *c = crep_get();

    _crep_init();
    log_fatalf("[%lu] abort on call '%s'\n", c->id, func);

    return NULL;
}
#endif

#define CP_BUF_SIZE 1024

static void
_cp(const char *dst, const char *src)
{
    FILE *dfp = rl_fopen(dst, "w");
    ASSERT(dfp);
    FILE *sfp = rl_fopen(src, "r");
    ASSERT(sfp);
    char buf[CP_BUF_SIZE];
    size_t rlen;
    while ((rlen = rl_fread(buf, 1, CP_BUF_SIZE, sfp))) {
        size_t wlen = rl_fwrite(buf, rlen, 1, dfp);
        ASSERT(wlen == 1);
    }
    int ret = rl_fclose(dfp);
    ASSERT(!ret);
    ret = rl_fclose(sfp);
    ASSERT(!ret);
}

void
_crep_clean(const char *sglob)
{
    glob_t pglob;
    int ret = glob(sglob, GLOB_NOSORT, NULL, &pglob);
    ASSERT(!ret || ret == GLOB_NOMATCH);
    for (size_t i = 0; i < pglob.gl_pathc; i++) {
        ret = remove(pglob.gl_pathv[i]);
        ASSERT(!ret);
    }
    globfree(&pglob);
}

void
crep_backup_make(void)
{
    crep_backup_clean();
    glob_t pglob;
    int ret = glob("task-*.crep", GLOB_NOSORT, NULL, &pglob);
    ASSERT(!ret || ret == GLOB_NOMATCH);
    for (size_t i = 0; i < pglob.gl_pathc; i++) {
        char fn[PATH_MAX];
        ret = rl_sprintf(fn, "%s.bak", pglob.gl_pathv[i]);
        ASSERT(ret < PATH_MAX);
        _cp(fn, pglob.gl_pathv[i]);
    }
    globfree(&pglob);
}

void
crep_backup_restore(void)
{
    _crep_clean("task-*.crep");
    glob_t pglob;
    int ret = glob("task-*.crep.bak", GLOB_NOSORT, NULL, &pglob);
    ASSERT(!ret || ret == GLOB_NOMATCH);
    for (size_t i = 0; i < pglob.gl_pathc; i++) {
        char fn[PATH_MAX];
        strcpy(fn, pglob.gl_pathv[i]);
        fn[rl_strlen(pglob.gl_pathv[i]) - 4] = '\0';
        _cp(fn, pglob.gl_pathv[i]);
    }
    globfree(&pglob);
}

void
crep_backup_clean(void)
{
    _crep_clean("task-*.crep.bak");
}

void
crep_truncate(clk_t clk)
{
    glob_t pglob;
    int ret = glob("task-*.crep", GLOB_NOSORT, NULL, &pglob);
    ASSERT(!ret || ret == GLOB_NOMATCH);
    for (size_t i = 0; i < pglob.gl_pathc; i++) {
        FILE *fp = rl_fopen(pglob.gl_pathv[i], "r");
        ASSERT(fp);
        size_t len = 0;
        for (callfun_t lfun; rl_fread(&lfun, sizeof(callfun_t), 1, fp) == 1;
             len += sizeof(callfun_t)) {
            if (lfun.clk >= clk) {
                break;
            }
            ASSERT(lfun.retsize <= LONG_MAX);
            ret = rl_fseek(fp, (long)lfun.retsize, SEEK_CUR);
            ASSERT(!ret);
            len += lfun.retsize;
            for (size_t j = 0; lfun.params[j].data && j < MAX_PARAMS; j++) {
                size_t s = lfun.params[j].size;
                ASSERT(s <= LONG_MAX);
                ret = rl_fseek(fp, (long)s, SEEK_CUR);
                ASSERT(!ret);
                len += s;
            }
        }
        ret = rl_fclose(fp);
        ASSERT(!ret);
        ASSERT(len <= LONG_MAX);
        ret = truncate(pglob.gl_pathv[i], (long)len);
        ASSERT(!ret);
    }
    globfree(&pglob);
}
