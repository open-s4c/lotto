/*
 */

#include <lotto/sys/stream_impl.h>
#include <lotto/util/redirect.h>

REDIRECT_RETURN(stream, size_t, write,
                (stream_t * stream, const char *buf, size_t size), stream,
                stream, buf, size)
REDIRECT_RETURN(stream, size_t, read,
                (stream_t * stream, char *buf, size_t size), stream, stream,
                buf, size)
REDIRECT_VOID(stream, close, (stream_t * stream), stream, stream)
REDIRECT_VOID(stream, reset, (stream_t * stream), stream, stream)
