#include <stdarg.h>
#include <stdint.h>

#include "foo.h"
#include "variadic_mock.h"

static mockoto_foo_13_f _foo_13_hook;
static uint64_t _foo_13_ret;
static int _foo_13_called;

void
mockoto_foo_13_hook(mockoto_foo_13_f cb)
{
    _foo_13_hook   = cb;
    _foo_13_called = 0;
}

void
mockoto_foo_13_returns(uint64_t ret)
{
    _foo_13_ret    = ret;
    _foo_13_called = 0;
}

int
mockoto_foo_13_called(void)
{
    return _foo_13_called;
}

uint64_t
foo_13(unsigned count, ...)
{
    uint64_t ret;
    va_list ap;

    _foo_13_called++;
    va_start(ap, count);
    if (_foo_13_hook != NULL)
        ret = _foo_13_hook(count, ap);
    else
        ret = _foo_13_ret;
    va_end(ap);
    return ret;
}

static mockoto_foo_14_f _foo_14_hook;
static double _foo_14_ret;
static int _foo_14_called;

void
mockoto_foo_14_hook(mockoto_foo_14_f cb)
{
    _foo_14_hook   = cb;
    _foo_14_called = 0;
}

void
mockoto_foo_14_returns(double ret)
{
    _foo_14_ret    = ret;
    _foo_14_called = 0;
}

int
mockoto_foo_14_called(void)
{
    return _foo_14_called;
}

double
foo_14(unsigned count, ...)
{
    double ret;
    va_list ap;

    _foo_14_called++;
    va_start(ap, count);
    if (_foo_14_hook != NULL)
        ret = _foo_14_hook(count, ap);
    else
        ret = _foo_14_ret;
    va_end(ap);
    return ret;
}
