/*
 */
#ifndef LOTTO_FORK_H
#define LOTTO_FORK_H
/**
 * @file fork.h
 * @brief The fork interface.
 *
 * Support of different implementations of fork depending on active plugin.
 */

#include <stdint.h>

/**
 * Wrapper around combination of fork() and execve() system calls.
 *
 * This function returns -1 on error, 0 in a parent process and does not return
 * for a child since it also calls execve().
 * If execve() itself fails, the child process panics.
 * On fork error, user can observe errno value set by fork().
 */
pid_t lotto_fork_execve(const char *pathname, char *const argv[],
                        char *const envp[]) __attribute__((weak));

#endif
