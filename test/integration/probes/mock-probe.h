#ifndef LOTTO_TEST_INTEGRATION_MOCK_PROBE_H
#define LOTTO_TEST_INTEGRATION_MOCK_PROBE_H

int lotto_test_handler_count(void);
int lotto_test_malloc_before_count(void);
int lotto_test_malloc_after_count(void);
int lotto_test_malloc_segfault_before_count(void);
int lotto_test_malloc_segfault_after_count(void);

#endif
