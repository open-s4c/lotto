#ifndef HANDLER_TIMEOUT_H
#define HANDLER_TIMEOUT_H

#include <lotto/base/task_id.h>
#include <lotto/engine/clock.h>

/**
 * Registers a deadline.
 *
 * @param id task id
 * @param deadline deadline
 */
void handler_timeout_register(task_id id, const struct timespec *deadline);
/**
 * Deregisters a deadline.
 *
 * @param id task id
 */
void handler_timeout_deregister(task_id id);

#endif
