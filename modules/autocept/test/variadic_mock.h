#ifndef LOTTO_MODULES_AUTOCEPT_TEST_VARIADIC_MOCK_H
#define LOTTO_MODULES_AUTOCEPT_TEST_VARIADIC_MOCK_H

#include <stdarg.h>
#include <stdint.h>

typedef uint64_t (*mockoto_foo_13_f)(unsigned count, va_list ap);
void mockoto_foo_13_hook(mockoto_foo_13_f cb);
void mockoto_foo_13_returns(uint64_t ret);
int mockoto_foo_13_called(void);

typedef double (*mockoto_foo_14_f)(unsigned count, va_list ap);
void mockoto_foo_14_hook(mockoto_foo_14_f cb);
void mockoto_foo_14_returns(double ret);
int mockoto_foo_14_called(void);

#endif
