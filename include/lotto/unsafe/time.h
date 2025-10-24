/*
 */
#ifndef LOTTO_UNSAFE_TIME_H
#define LOTTO_UNSAFE_TIME_H
/**
 * @file time.h
 * @brief The time interface.
 *
 * The time interface allows the user to call the real time function. This
 * function will always return the current time even if lotto is in replay mode,
 * so the returned values will be different during replay. This is an unsafe
 * interface and the user is not recommended to use it.
 */

#include <time.h>

/**
 * Returns the real time
 */
time_t real_time(time_t *tloc) __attribute__((weak));

#endif
