/*
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <lotto/sys/assert.h>
#include <lotto/sys/fcntl.h>
#include <lotto/sys/logger.h>
#include <lotto/sys/stdlib.h>
#include <lotto/sys/stream_file.h>
#include <lotto/sys/stream_impl.h>
#include <lotto/sys/unistd.h>

typedef struct {
    stream_t s;
    int fd;
} stream_file_t;

static size_t
stream_file_write(stream_t *stream, const char *buf, size_t size)
{
    stream_file_t *stream_file = (stream_file_t *)stream;
    return sys_write(stream_file->fd, buf, size);
}

static size_t
stream_file_read(stream_t *stream, char *buf, size_t size)
{
    stream_file_t *stream_file = (stream_file_t *)stream;
    return sys_read(stream_file->fd, buf, size);
}

static void
stream_file_close(stream_t *stream)
{
    stream_file_t *stream_file = (stream_file_t *)stream;
    sys_close(stream_file->fd);
}

static void
stream_file_reset(stream_t *stream)
{
    stream_file_t *stream_file = (stream_file_t *)stream;
    sys_lseek(stream_file->fd, 0, SEEK_SET);
}

stream_t *
stream_file_alloc()
{
    stream_t *result = sys_malloc(sizeof(stream_file_t));
    ASSERT(result);
    return result;
}

void
stream_file_init(stream_t *s, int fd)
{
    stream_file_t *stream = (stream_file_t *)s;
    stream->fd            = fd;
    stream->s.read        = stream_file_read;
    stream->s.write       = stream_file_write;
    stream->s.close       = stream_file_close;
    stream->s.reset       = stream_file_reset;
}

void
stream_file_in(stream_t *s, const char *fname)
{
    int fd = sys_open(fname, O_RDONLY, 0);
    if (fd == -1)
        log_fatalf("error: could not open file '%s'\n", fname);

    stream_file_init(s, fd);
}

void
stream_file_out(stream_t *s, const char *fname)
{
    int fd = sys_open(fname, O_CREAT | O_WRONLY | O_TRUNC,
                      S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    if (fd == -1)
        log_fatalf("error: could not open file '%s'\n", fname);

    stream_file_init(s, fd);
}
