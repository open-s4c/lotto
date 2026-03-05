#ifndef LOTTO_SIGNATURES_IO_H
#define LOTTO_SIGNATURES_IO_H

#include <stdarg.h>
#include <stdio.h>

#include <lotto/sys/signatures/defaults_head.h>

#define SYS_FDOPEN                                                             \
    SYS_FUNC(LIBC, return,                                                     \
             SIG(FILE *, fdopen, int, fildes, const char *, mode), )
#define SYS_FOPEN                                                              \
    SYS_FUNC(LIBC, return,                                                     \
             SIG(FILE *, fopen, const char *, pathname, const char *, mode), )
#define SYS_FCLOSE SYS_FUNC(LIBC, return, SIG(int, fclose, FILE *, stream), )
#define SYS_FSEEK                                                              \
    SYS_FUNC(LIBC, return,                                                     \
             SIG(int, fseek, FILE *, stream, long int, off, int, whence), )
#define SYS_FTELL SYS_FUNC(LIBC, return, SIG(long int, ftell, FILE *, stream), )
#define SYS_FREAD                                                              \
    SYS_FUNC(LIBC, return,                                                     \
             SIG(size_t, fread, void *, ptr, size_t, size, size_t, n, FILE *,  \
                 stream), )
#define SYS_FWRITE                                                             \
    SYS_FUNC(LIBC, return,                                                     \
             SIG(size_t, fwrite, const void *, ptr, size_t, size, size_t, n,   \
                 FILE *, stream), )
#define SYS_REWIND SYS_FUNC(LIBC, , SIG(void, rewind, FILE *, stream), )


#define SYS_GETLINE                                                            \
    SYS_FUNC(LIBC, return,                                                     \
             SIG(ssize_t, getline, char **, lineptr, size_t *, n, FILE *,      \
                 stream), )
#define SYS_GETDELIM                                                           \
    SYS_FUNC(LIBC, return,                                                     \
             SIG(ssize_t, getdelim, char **, lineptr, size_t *, n, int, delim, \
                 FILE *, stream), )
#define SYS_SETVBUF                                                            \
    SYS_FUNC(LIBC, return,                                                     \
             SIG(int, setvbuf, FILE *, stream, char *, buf, int, type, size_t, \
                 size), )

#define SYS_PRINTF                                                             \
    SYS_FORMAT_FUNC(return, SIG(int, printf, const char *, fmt), PLF(1))
#define SYS_VPRINTF                                                            \
    SYS_FUNC(LIBC, return,                                                     \
             SIG(int, vprintf, const char *, format, va_list, ap), )

#define SYS_DPRINTF                                                            \
    SYS_FORMAT_FUNC(return, SIG(int, dprintf, int, fd, const char *, fmt),     \
                          PLF(2))
#define SYS_VDPRINTF                                                           \
    SYS_FUNC(                                                                  \
        LIBC, return,                                                          \
        SIG(int, vdprintf, int, fd, const char *, format, va_list, arg), )


#define SYS_FPRINTF                                                            \
    SYS_FORMAT_FUNC(                                                           \
        return, SIG(int, fprintf, FILE *, stream, const char *, fmt), PLF(2))
#define SYS_VFPRINTF                                                           \
    SYS_FUNC(LIBC, return,                                                     \
             SIG(int, vfprintf, FILE *, stream, const char *, format, va_list, \
                 arg), )


#define SYS_SPRINTF                                                            \
    SYS_FORMAT_FUNC(return, SIG(int, sprintf, char *, str, const char *, fmt), \
                          PLF(2))
#define SYS_VSPRINTF                                                           \
    SYS_FUNC(                                                                  \
        LIBC, return,                                                          \
        SIG(int, vsprintf, char *, s, const char *, format, va_list, ap), )

#define SYS_SNPRINTF                                                           \
    SYS_FORMAT_FUNC(return,                                                    \
                          SIG(int, snprintf, char *, str, size_t, size,        \
                              const char *, fmt),                              \
                          PLF(3))
#define SYS_VSNPRINTF                                                          \
    SYS_FUNC(LIBC, return,                                                     \
             SIG(int, vsnprintf, char *, s, size_t, n, const char *, format,   \
                 va_list, ap), )

#define SYS_SCANF                                                              \
    SYS_FORMAT_FUNC(return, SIG(int, scanf, const char *, fmt), SLF(1))
#define SYS_VSCANF                                                             \
    SYS_FUNC(LIBC, return,                                                     \
             SIG(int, vscanf, const char *, format, va_list, ap), )

#define SYS_FSCANF                                                             \
    SYS_FORMAT_FUNC(                                                           \
        return, SIG(int, fscanf, FILE *, stream, const char *, fmt), SLF(2))
#define SYS_VFSCANF                                                            \
    SYS_FUNC(LIBC, return,                                                     \
             SIG(int, vfscanf, FILE *, stream, const char *, format, va_list,  \
                 ap), )

#define SYS_SSCANF                                                             \
    SYS_FORMAT_FUNC(                                                           \
        return, SIG(int, sscanf, const char *, str, const char *, fmt),        \
              SLF(2))
#define SYS_VSSCANF                                                            \
    SYS_FUNC(LIBC, return,                                                     \
             SIG(int, vsscanf, const char *, str, const char *, format,        \
                 va_list, ap), )

#define FOR_EACH_SYS_STDIO_WRAPPED                                             \
    SYS_FDOPEN                                                                 \
    SYS_FOPEN                                                                  \
    SYS_FCLOSE                                                                 \
    SYS_FSEEK                                                                  \
    SYS_FTELL                                                                  \
    SYS_FREAD                                                                  \
    SYS_FWRITE                                                                 \
    SYS_GETLINE                                                                \
    SYS_GETDELIM                                                               \
    SYS_SETVBUF                                                                \
    SYS_PRINTF                                                                 \
    SYS_VPRINTF                                                                \
    SYS_DPRINTF                                                                \
    SYS_VDPRINTF                                                               \
    SYS_FPRINTF                                                                \
    SYS_VFPRINTF                                                               \
    SYS_SPRINTF                                                                \
    SYS_VSPRINTF                                                               \
    SYS_SNPRINTF                                                               \
    SYS_VSNPRINTF                                                              \
    SYS_SCANF                                                                  \
    SYS_VSCANF                                                                 \
    SYS_FSCANF                                                                 \
    SYS_VFSCANF                                                                \
    SYS_SSCANF                                                                 \
    SYS_VSSCANF

#define FOR_EACH_SYS_STDIO_CUSTOM SYS_REWIND

#define FOR_EACH_SYS_STDIO                                                     \
    FOR_EACH_SYS_STDIO_WRAPPED                                                 \
    FOR_EACH_SYS_STDIO_CUSTOM

#endif // LOTTO_SIGNATURES_IO_H
