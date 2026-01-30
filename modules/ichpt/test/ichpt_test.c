#include <lotto/brokers/statemgr.h>
#include <lotto/engine/dispatcher.h>
#include <lotto/engine/handlers/ichpt.h>
#include <lotto/sys/ensure.h>
#include <lotto/sys/stream_impl.h>
#include <lotto/sys/string.h>

void ichpt_reset();

#define I(V) ((uintptr_t)(V))
#define I1   I(1)
#define I2   I(2)
#define I3   I(3)

void
dispatcher_register(slot_t slot, handle_f handle)
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
void
test_add()
{
    ichpt_reset();
    ENSURE(!is_ichpt(I1));
    ENSURE(!is_ichpt(I2));
    add_ichpt(I2);
    ENSURE(!is_ichpt(I1));
    ENSURE(is_ichpt(I2));
    add_ichpt(I1);
    ENSURE(is_ichpt(I1));
    ENSURE(is_ichpt(I2));
}

void
test_add_again()
{
    ichpt_reset();
    add_ichpt(I2);
    add_ichpt(I1);
    add_ichpt(I1);
    add_ichpt(I2);
    add_ichpt(I1);
    ENSURE(ichpt_count() == 2);
}

/*******************************************************************************
 * test saving and restoring the list of ichpts
 ******************************************************************************/

#define BUF_SIZE 2048

char test_buf[BUF_SIZE];
size_t test_write_count;
size_t test_read_count;

size_t
_buf_write(stream_t *stream, const char *buf, size_t size)
{
    char *dst = test_buf + test_write_count;
    ENSURE((test_write_count + size + 1) <= BUF_SIZE);
    sys_memcpy(dst, buf, size);
    dst[size] = '\0';
    test_write_count += size;
    return size;
}

size_t
_buf_read(stream_t *stream, char *buf, size_t size)
{
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


int
main()
{
    test_add();
    test_add_again();
    return 0;
}
