#include <lotto/brokers/statemgr.h>
#include <lotto/engine/dispatcher.h>
#include <lotto/engine/handlers/race.h>
#include <lotto/sys/ensure.h>
#include <lotto/sys/string.h>

void race_reset();
race_t race_check(const context_t *ctx, clk_t clk);

#define A(V) ((uintptr_t)(V))
#define I1   A(1)
#define I2   A(2)
#define I3   A(3)

#define A1 A(101)
#define A2 A(102)
#define A3 A(103)
#define A4 A(104)

void
dispatcher_register(slot_t slot, handle_f handle)
{
}

void
add_ichpt(uintptr_t addr)
{
}

#undef statemgr_register
void
statemgr_register(marshable_t *m, state_type_t type)
{
}

/*******************************************************************************
 * simple test for adding and checkign ichpts
 ******************************************************************************/
typedef struct {
    context_t ctx;
    race_t race;
    bool end;
} call_t;
#define END                                                                    \
    (call_t)                                                                   \
    {                                                                          \
        .end = true                                                            \
    }

#define race(a, i1, i2)                                                        \
    (race_t)                                                                   \
    {                                                                          \
        .addr = (a), .loc1 = (struct race_loc){.pc = (i1)},                    \
        .loc2 = (struct race_loc){.pc = (i2)},                                 \
    }

#define actx(ID, ADDR, CAT, PC)                                                \
    (context_t)                                                                \
    {                                                                          \
        .id = (ID), .pc = (PC), .cat = (CAT),                                  \
        .args[0] = (arg_t){.width = ARG_PTR, .value.ptr = (ADDR)},             \
    }

#define NORACE race(0, 0, 0)

void
test_add()
{
    task_id t1 = 1;
    task_id t2 = 2;

    call_t calls[] = {
        {actx(t1, CAT_BEFORE_READ, A1, I1), NORACE},
        {actx(t2, CAT_BEFORE_WRITE, A1, I2), NORACE},
        END,
    };

    for (call_t *c = calls; !c->end; c++) {
        race_t r = race_check(&c->ctx, 0);
        ENSURE(r.addr == c->race.addr);
        ENSURE(r.loc1.pc == c->race.loc2.pc);
    }
}
/*******************************************************************************
 * test saving and restoring the list of ichpts
 ******************************************************************************/

#define BUF_SIZE 2048

char test_buf[BUF_SIZE];
size_t test_write_count;
size_t test_read_count;

size_t
_buf_write(void *arg, const char *buf, size_t size)
{
    (void)arg;
    char *dst = test_buf + test_write_count;
    ENSURE((test_write_count + size + 1) <= BUF_SIZE);
    sys_memcpy(dst, buf, size);
    dst[size] = '\0';
    test_write_count += size;
    return size;
}

size_t
_buf_read(void *arg, char *buf, size_t size)
{
    (void)arg;
    ENSURE((test_read_count + size + 1) <= BUF_SIZE);

    if (test_read_count + size > test_write_count)
        size = test_write_count - test_read_count;

    if (size == 0)
        return 0;

    char *src = test_buf + test_read_count;
    sys_memcpy(buf, src, size);

    test_read_count += size;
    return size;
}

#if 0
void
test_save_load()
{
    /* add this ichpts */
    ichpt_reset();
    add_ichpt(I2);
    add_ichpt(I1);
    ENSURE(is_ichpt(I1));
    ENSURE(is_ichpt(I2));
    ENSURE(!is_ichpt(I3));

    /* save ichpts to a file */
    stream_t s = {
        .write = _buf_write,
        .read  = _buf_read,
        .close = NULL,
        .arg   = NULL,
    };
    ichpt_save(s);

    /* clear ichpts and load stream back */
    ichpt_reset();
    ENSURE(!is_ichpt(I1));
    ENSURE(!is_ichpt(I2));
    ichpt_load(s);
    ENSURE(is_ichpt(I1));
    ENSURE(is_ichpt(I2));
    ENSURE(!is_ichpt(I3));
}
#endif

int
main()
{
    test_add();
    // test_save_load();
    return 0;
}
