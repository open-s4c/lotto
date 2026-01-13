/*
 */

#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <lotto/sys/assert.h>
#include <lotto/sys/logger.h>
#include <lotto/sys/stdio.h>
#include <lotto/sys/stdlib.h>
#include <lotto/sys/stream_chunked_impl.h>

typedef struct {
    stream_chunked_t s;
    FILE *current_fp;
    uint64_t total_files;
    uint64_t current_file;
    DIR *dp;
    const char *dir_path;
    const char *suffix;
    bool read_only;
} stream_chunked_file_t;

static void
_write_chunk_filename(char *dst, const stream_chunked_file_t *stream,
                      uint64_t chunk)
{
    sys_sprintf(dst, "%s/%lu%s", stream->dir_path, chunk, stream->suffix);
}

static void
_write_filename(char *dst, const stream_chunked_file_t *stream)
{
    _write_chunk_filename(dst, stream, stream->current_file);
}

static void
_write_next_file(stream_chunked_file_t *stream)
{
    if (stream->current_fp != NULL) {
        fflush(stream->current_fp);
        sys_fclose(stream->current_fp);
        stream->current_file++;
    }
    char filename[PATH_MAX];
    _write_filename(filename, stream);
    stream->current_fp = sys_fopen(filename, "w");
    stream->read_only  = false;
    ASSERT(stream->current_fp);
    if (stream->current_fp == NULL)
        logger_fatalf("could not open %s\n", filename);
    stream->total_files++;
}

static void
_scan_files(stream_chunked_file_t *stream)
{
    struct dirent *file;
    int64_t max_chunk = -1;
    rewinddir(stream->dp);
    for (rewinddir(stream->dp); (file = readdir(stream->dp));) {
        if (!strcmp(file->d_name, ".") || !strcmp(file->d_name, ".."))
            continue;
        int64_t current_chunk = atoi(file->d_name);
        max_chunk = current_chunk > max_chunk ? current_chunk : max_chunk;
    }
    stream->total_files = max_chunk + 1;
}

static void
_read_next_file(stream_chunked_file_t *stream)
{
    if (stream->total_files == 0) {
        ASSERT(stream->current_fp == NULL);
        _scan_files(stream);
    }
    // ASSERT(stream->total_files > stream->current_file);
    if (stream->total_files <= stream->current_file)
        return;
    if (stream->current_fp != NULL) {
        sys_fclose(stream->current_fp);
        stream->current_fp = NULL;
        stream->current_file++;
    }

    if (stream->current_file > stream->total_files - 1) {
        return;
    }
    char filename[PATH_MAX];
    _write_filename(filename, stream);
    stream->current_fp = sys_fopen(filename, "r");
    stream->read_only  = true;
    if (stream->current_fp == NULL)
        logger_fatalf("could not open %s\n", filename);
}

static size_t
_write(stream_t *stream, const char *buf, size_t size)
{
    stream_chunked_file_t *stream_file = (stream_chunked_file_t *)stream;
    if (stream_file->current_fp == NULL) {
        _write_next_file(stream_file);
    }
    if (stream_file->read_only) {
        stream_file->current_file--;
        _write_next_file(stream_file);
    }
    return sys_fwrite(buf, sizeof(char), size, stream_file->current_fp);
}

static size_t
_read(stream_t *stream, char *buf, size_t size)
{
    stream_chunked_file_t *stream_file = (stream_chunked_file_t *)stream;
    if (stream_file->total_files == 0) {
        _read_next_file(stream_file);
    }
    return stream_file->current_fp == NULL ?
               0 :
               sys_fread(buf, sizeof(char), size, stream_file->current_fp);
}

static void
_close(stream_t *stream)
{
    stream_chunked_file_t *stream_file = (stream_chunked_file_t *)stream;
    if (stream_file->current_fp != NULL) {
        fflush(stream_file->current_fp);
        sys_fclose(stream_file->current_fp);
    }
    closedir(stream_file->dp);
    sys_free(stream);
}

static uint64_t
_nchunks(stream_t *stream)
{
    stream_chunked_file_t *stream_file = (stream_chunked_file_t *)stream;
    if (stream_file->total_files == 0) {
        _scan_files(stream_file);
    }
    return stream_file->total_files;
}

static bool
_set_chunk(stream_t *stream, uint64_t chunk)
{
    stream_chunked_file_t *stream_file = (stream_chunked_file_t *)stream;
    if (stream_file->current_file == chunk && stream_file->current_fp) {
        return true;
    }
    if (stream_file->total_files == 0) {
        _scan_files(stream_file);
    }
    if (chunk >= stream_file->total_files) {
        return false;
    }
    if (stream_file->current_fp) {
        fflush(stream_file->current_fp);
        sys_fclose(stream_file->current_fp);
    }
    stream_file->current_file = chunk;
    char filename[PATH_MAX];
    _write_filename(filename, stream_file);
    stream_file->current_fp = sys_fopen(filename, "r");
    stream_file->read_only  = true;
    if (stream_file->current_fp == NULL)
        logger_fatalf("could not open %s\n", filename);
    return true;
}

static bool
_next_chunk(stream_t *stream)
{
    stream_chunked_file_t *stream_file = (stream_chunked_file_t *)stream;
    if (!stream_file->current_fp) {
        return true;
    }
    fflush(stream_file->current_fp);
    sys_fclose(stream_file->current_fp);
    stream_file->current_fp = NULL;
    stream_file->current_file++;
    return true;
}

static void
_copy_chunks(const stream_t *source_stream, stream_t *sink_stream,
             uint64_t first_chunk, uint64_t last_chunk)
{
    ASSERT(first_chunk <= last_chunk);
    const stream_chunked_file_t *source_file_stream =
        (const stream_chunked_file_t *)source_stream;
    stream_chunked_file_t *sink_file_stream =
        (stream_chunked_file_t *)sink_stream;
    if (sink_file_stream->total_files == 0) {
        _scan_files(sink_file_stream);
    }
    if (strcmp(source_file_stream->dir_path, sink_file_stream->dir_path) == 0) {
        return;
    }
    ASSERT(source_file_stream->total_files > last_chunk);
    for (uint64_t i = first_chunk; i <= last_chunk; i++) {
        char source_filename[PATH_MAX];
        _write_chunk_filename(source_filename, source_file_stream, i);
        char target_filename[PATH_MAX];
        _write_chunk_filename(target_filename, sink_file_stream, i);
        FILE *source_fp = sys_fopen(source_filename, "r");
        FILE *sink_fp   = sys_fopen(target_filename, "w");
        char buf[BUFSIZ];

        while (!feof(source_fp)) {
            sys_fwrite(buf, 1, sys_fread(buf, 1, sizeof(buf), source_fp),
                       sink_fp);
        }
        sys_fclose(source_fp);
        sys_fclose(sink_fp);
    }
    if (sink_file_stream->total_files <= last_chunk) {
        sink_file_stream->total_files = last_chunk + 1;
    }
}

static void
_truncate(const stream_t *stream, uint64_t chunk_limit)
{
    stream_chunked_file_t *file_stream = (stream_chunked_file_t *)stream;
    if (file_stream->total_files == 0) {
        _scan_files(file_stream);
    }
    if (file_stream->total_files <= chunk_limit) {
        return;
    }
    if (file_stream->current_fp) {
        fflush(file_stream->current_fp);
        sys_fclose(file_stream->current_fp);
        file_stream->current_fp = NULL;
    }
    for (; chunk_limit < file_stream->total_files; file_stream->total_files--) {
        char filename[PATH_MAX];
        _write_chunk_filename(filename, file_stream,
                              file_stream->total_files - 1);
        remove(filename);
    }
    file_stream->current_file =
        file_stream->total_files > 0 ? file_stream->total_files - 1 : 0;
}

stream_t *
stream_chunked_file(DIR *dp, const char *dir_path, const char *suffix)
{
    stream_chunked_file_t *stream = sys_malloc(sizeof(stream_chunked_file_t));
    stream->dp                    = dp;
    stream->dir_path              = dir_path;
    stream->suffix                = suffix;
    stream->current_file          = 0;
    stream->total_files           = 0;
    stream->s.s.read              = _read;
    stream->s.s.write             = _write;
    stream->s.s.close             = _close;
    stream->s.nchunks             = _nchunks;
    stream->s.next_chunk          = _next_chunk;
    stream->s.set_chunk           = _set_chunk;
    stream->s.copy_chunks         = _copy_chunks;
    stream->s.truncate            = _truncate;
    stream->current_fp            = NULL;

    return (stream_t *)stream;
}
