/*
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <lotto/sys/assert.h>
#include <lotto/sys/logger.h>
#include <lotto/sys/stdio.h>
#include <lotto/sys/stdlib.h>
#include <lotto/sys/stream_ffile.h>
#include <lotto/sys/stream_impl.h>

typedef struct {
    stream_t s;
    FILE *fp;
} stream_ffile_t;

static size_t
stream_ffile_write(stream_t *stream, const char *buf, size_t size)
{
    stream_ffile_t *stream_ffile = (stream_ffile_t *)stream;
    return sys_fwrite(buf, sizeof(char), size, stream_ffile->fp);
}

static size_t
stream_ffile_read(stream_t *stream, char *buf, size_t size)
{
    stream_ffile_t *stream_ffile = (stream_ffile_t *)stream;
    return sys_fread(buf, sizeof(char), size, stream_ffile->fp);
}

static void
stream_ffile_close(stream_t *stream)
{
    stream_ffile_t *stream_ffile = (stream_ffile_t *)stream;
    fflush(stream_ffile->fp);
    sys_fclose(stream_ffile->fp);
}

stream_t *
stream_ffile_alloc()
{
    stream_t *result = sys_malloc(sizeof(stream_ffile_t));
    ASSERT(result);
    return result;
}

void
stream_ffile_init(stream_t *s, FILE *fp)
{
    stream_ffile_t *stream = (stream_ffile_t *)s;
    stream->fp             = fp;
    stream->s.read         = stream_ffile_read;
    stream->s.write        = stream_ffile_write;
    stream->s.close        = stream_ffile_close;
}

void
stream_ffile_in(stream_t *s, const char *fname)
{
    FILE *fp = sys_fopen(fname, "r+");
    if (fp == NULL)
        log_fatalf("error: could not open file '%s'\n", fname);

    stream_ffile_init(s, fp);
}

void
stream_ffile_out(stream_t *s, const char *fname)
{
    FILE *fp = sys_fopen(fname, "w+");
    if (fp == NULL)
        log_fatalf("error: could not open file '%s'\n", fname);

    stream_ffile_init(s, fp);
}
