#define LOTTO_REAL_NEXT

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lotto/recorder/recorder_flat.h>
#include <lotto/sys/stream_impl.h>

#define BUF_SIZE 2048

char test_buf[BUF_SIZE];
size_t test_write_count;
size_t test_read_count;

size_t
test_buf_write(stream_t *stream, const char *buf, size_t size)
{
    char *dst = test_buf + test_write_count;
    assert((test_write_count + size + 1) <= BUF_SIZE);
    memcpy(dst, buf, size);
    dst[size] = '\0';
    test_write_count += size;
    return 1;
}

size_t
test_buf_read(stream_t *stream, char *buf, size_t size)
{
    assert((test_read_count + size + 1) <= BUF_SIZE);

    if (test_read_count + size > test_write_count)
        size = test_write_count - test_read_count;

    if (size == 0)
        return 0;

    char *src = test_buf + test_read_count;
    memcpy(buf, src, size);

    test_read_count += size;
    return size;
}

int
main()
{
    stream_t stream = {.write = test_buf_write,
                       .read  = test_buf_read,
                       .close = NULL};

    trace_t *recorder = recorder_flat_create(&stream);

    const char *msg = "hello";

    record_t *r = record_alloc(strlen(msg));
    r->kind     = RECORD_INFO;
    int v       = trace_append(recorder, r);
    assert(v == RECORDER_OK);
    memcpy(r->data, msg, strlen(msg));

    recorder_save(recorder);
    r = (record_t *)test_buf;
    assert(strcmp(r->data, "hello") == 0);

    recorder = recorder_flat_create(&stream);
    recorder_load(recorder);

    record_t *record = trace_next(recorder, RECORD_ANY);
    assert(record != NULL);
    printf("%s\n", record->data);
    trace_advance(recorder);

    return 0;
}
