#define LOTTO_REAL_NEXT

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <lotto/base/trace_chunked.h>
#include <lotto/sys/stream_chunked_impl.h>

#define CHUNK_SIZE 4096
#define NCHUNKS    128

typedef struct {
    stream_chunked_t s;
    char test_buf[NCHUNKS][CHUNK_SIZE];
    size_t test_write_count[NCHUNKS];
    size_t test_read_count[NCHUNKS];
    uint64_t current_chunk;
    uint64_t total_chunks;
    bool touched;
} stream_chunked_mock_t;

typedef void(test_f)(stream_chunked_mock_t *);

size_t
mock_stream_write(stream_t *stream, const char *buf, size_t size)
{
    stream_chunked_mock_t *mock = (stream_chunked_mock_t *)stream;
    if (!mock->touched) {
        mock->test_write_count[mock->current_chunk] = 0;
        mock->touched                               = true;
    }
    char *dst = mock->test_buf[mock->current_chunk] +
                mock->test_write_count[mock->current_chunk];
    assert((mock->test_write_count[mock->current_chunk] + size) <= CHUNK_SIZE);
    memcpy(dst, buf, size);
    mock->test_write_count[mock->current_chunk] += size;
    return size;
}

size_t
mock_stream_read(stream_t *stream, char *buf, size_t size)
{
    stream_chunked_mock_t *mock = (stream_chunked_mock_t *)stream;
    assert((mock->test_read_count[mock->current_chunk] + size) <= CHUNK_SIZE);

    if (mock->test_read_count[mock->current_chunk] + size >
        mock->test_write_count[mock->current_chunk])
        size = mock->test_write_count[mock->current_chunk] -
               mock->test_read_count[mock->current_chunk];

    char *src = mock->test_buf[mock->current_chunk] +
                mock->test_read_count[mock->current_chunk];
    memcpy(buf, src, size);

    mock->test_read_count[mock->current_chunk] += size;
    assert(mock->test_read_count[mock->current_chunk] <=
           mock->test_write_count[mock->current_chunk]);
    return size;
}

bool
mock_stream_set_chunk(stream_t *stream, uint64_t chunk)
{
    stream_chunked_mock_t *mock = (stream_chunked_mock_t *)stream;
    if (mock->total_chunks <= chunk) {
        return false;
    }
    mock->test_read_count[mock->current_chunk] = 0;
    mock->current_chunk                        = chunk;
    mock->touched                              = false;
    return true;
}

bool
mock_stream_next_chunk(stream_t *stream)
{
    stream_chunked_mock_t *mock = (stream_chunked_mock_t *)stream;
    mock->test_read_count[mock->current_chunk] = 0;
    mock->current_chunk++;
    mock->touched = false;
    assert(mock->current_chunk < NCHUNKS);
    if (mock->current_chunk >= mock->total_chunks) {
        mock->total_chunks = mock->current_chunk + 1;
    }
    return true;
}

uint64_t
mock_stream_nchunks(stream_t *stream)
{
    stream_chunked_mock_t *mock = (stream_chunked_mock_t *)stream;
    return mock->total_chunks;
}

void
mock_stream_chunked_truncate(const stream_t *stream, uint64_t chunk_limit)
{
    stream_chunked_mock_t *mock = (stream_chunked_mock_t *)stream;
    for (uint64_t i = chunk_limit; i < NCHUNKS; i++) {
        mock->test_read_count[i] = mock->test_write_count[i] = 0;
    }
    mock->total_chunks = chunk_limit;
}

void
mock_stream_close(stream_t *stream)
{
    stream_chunked_mock_t *mock = (stream_chunked_mock_t *)stream;
    for (uint64_t i = 0; i < NCHUNKS; i++) {
        mock->test_read_count[i] = 0;
    }
    mock->current_chunk = 0;
}

void
initialize_mock(stream_chunked_mock_t *mock)
{
    memset(mock, 0, sizeof(stream_chunked_mock_t));
    mock->s = (stream_chunked_t){.nchunks    = mock_stream_nchunks,
                                 .next_chunk = mock_stream_next_chunk,
                                 .set_chunk  = mock_stream_set_chunk,
                                 .truncate   = mock_stream_chunked_truncate,
                                 .s.read     = mock_stream_read,
                                 .s.write    = mock_stream_write,
                                 .s.close    = mock_stream_close};
}

uint64_t
rand64()
{
    return (((uint64_t)rand()) << 32) + rand();
}

void
write_test(stream_chunked_mock_t *mock)
{
    trace_t *recorder = trace_chunked_create(&mock->s.s, 1);
    const char *msg1  = "first";
    record_t *r       = record_alloc(strlen(msg1) + 1);
    r->kind           = RECORD_INFO;
    memcpy(r->data, msg1, strlen(msg1) + 1);
    int v = trace_append(recorder, r);
    assert(v == TRACE_OK);
    const char *msg2 = "second";
    r                = record_alloc(strlen(msg2) + 1);
    r->kind          = RECORD_INFO;
    memcpy(r->data, msg2, strlen(msg2) + 1);
    v = trace_append(recorder, r);
    assert(v == TRACE_OK);
    // first chunk is already written
    r = (record_t *)mock->test_buf[0];
    assert(strcmp(r->data, msg1) == 0);
    trace_save(recorder);
    trace_destroy(recorder);
    r = (record_t *)mock->test_buf[1];
    assert(strcmp(r->data, msg2) == 0);
}

void
read_test(stream_chunked_mock_t *mock)
{
    write_test(mock);
    mock->current_chunk = 0;
    const char *msg1    = "first";
    const char *msg2    = "second";
    trace_t *recorder   = trace_chunked_create(&mock->s.s, 1);
    trace_load(recorder);
    record_t *r = trace_next(recorder, RECORD_ANY);
    assert(r);
    assert(strcmp(r->data, msg1) == 0);
    trace_advance(recorder);
    r = trace_next(recorder, RECORD_ANY);
    assert(r);
    assert(strcmp(r->data, msg2) == 0);
    trace_destroy(recorder);
}

static void
_stress_fill(stream_chunked_mock_t *mock, trace_t *recorder,
             uint64_t total_records)
{
    for (uint64_t i = 0; i < total_records; i++) {
        uint64_t wiggle = rand64() % 32 + 1;
        for (uint64_t j = 0; j < wiggle; j++) {
            record_t *r = record_alloc(0);
            r->kind     = RECORD_INFO;
            r->clk      = 0;
            assert(trace_append(recorder, r) == TRACE_OK);
        }
        for (uint64_t j = 0; j < wiggle; j++) {
            trace_forget(recorder);
        }
        record_t *r = record_alloc(0);
        r->kind     = RECORD_INFO;
        r->clk      = i + 1;
        assert(trace_append(recorder, r) == TRACE_OK);
    }
}

static void
_stress_check(stream_chunked_mock_t *mock, trace_t *recorder,
              uint64_t total_records)
{
    for (uint64_t i = 0; i < total_records; i++) {
        record_t *r = trace_next(recorder, RECORD_ANY);
        assert(r && r->clk == i + 1);
        trace_advance(recorder);
    }
    assert(!trace_next(recorder, RECORD_ANY));
}

void
stress_update_test(stream_chunked_mock_t *mock)
{
    uint64_t chunk_size    = rand64() % 8 + 1;
    uint64_t total_records = rand64() % 32 + 1;
    trace_t *recorder      = trace_chunked_create(&mock->s.s, chunk_size);
    _stress_fill(mock, recorder, total_records);
    _stress_check(mock, recorder, total_records);
}

void
stress_reload_test(stream_chunked_mock_t *mock)
{
    uint64_t chunk_size    = rand64() % 8 + 1;
    uint64_t total_records = rand64() % 32 + 1;
    trace_t *recorder      = trace_chunked_create(&mock->s.s, chunk_size);
    _stress_fill(mock, recorder, total_records);
    trace_save(recorder);
    trace_destroy(recorder);
    stream_close(&mock->s.s);
    recorder = trace_chunked_create(&mock->s.s, chunk_size);
    trace_load(recorder);
    _stress_check(mock, recorder, total_records);
}

int
main()
{
    test_f *tests[]   = {write_test, read_test, stress_update_test,
                         stress_reload_test, NULL};
    unsigned int seed = (unsigned int)time(NULL);
    printf("Seed: %d\n", seed);
    srand(seed);
    for (test_f **test = tests; *test; test++) {
        stream_chunked_mock_t stream;
        initialize_mock(&stream);
        (*test)(&stream);
    }
    return 0;
}
