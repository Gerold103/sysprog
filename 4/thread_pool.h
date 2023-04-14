#pragma once

#include <stdbool.h>

/**
 * Here you should specify which features do you want to implement via macros:
 * NEED_DETACH and NEED_TIMED_JOIN. If you want to enable detach, do:
 *
 *     #define NEED_DETACH
 *
 * To enable timed join do:
 *
 *     #define NEED_TIMED_JOIN
 *
 * It is important to define these macros here, in the header, because it is
 * used by tests.
 */

struct thread_pool;
struct thread_task;

typedef void *(*thread_task_f)(void *);

enum {
	TPOOL_MAX_THREADS = 20,
	TPOOL_MAX_TASKS = 100000,
};

enum thread_poool_errcode {
	TPOOL_ERR_INVALID_ARGUMENT = 1,
	TPOOL_ERR_TOO_MANY_TASKS,
	TPOOL_ERR_HAS_TASKS,
	TPOOL_ERR_TASK_NOT_PUSHED,
	TPOOL_ERR_TASK_IN_POOL,
	TPOOL_ERR_NOT_IMPLEMENTED,
	TPOOL_ERR_TIMEOUT,
};

/** Thread pool API. */

/**
 * Create a new thread pool with maximum @a max_thread_count
 * threads.
 * @param max_thread_count Maximum pool size.
 * @param[out] Pointer to store result pool object.
 *
 * @retval 0 Success.
 * @retval != 0 Error code.
 *     - TPOOL_ERR_INVALID_ARGUMENT - max_thread_count is too big,
 *       or 0.
 */
int
thread_pool_new(int max_thread_count, struct thread_pool **pool);

/**
 * How many threads are created by this pool. Can be less than
 * max.
 * @param pool Thread pool to get thread count of.
 * @retval Thread count.
 */
int
thread_pool_thread_count(const struct thread_pool *pool);

/**
 * Delete @a pool, free its memory.
 * @param pool Pool to delete.
 * @retval 0 Success.
 * @retval != Error code.
 *     - TPOOL_ERR_HAS_TASKS - pool still has tasks.
 */
int
thread_pool_delete(struct thread_pool *pool);

/**
 * Push @a task into thread pool queue.
 * @param pool Pool to push into.
 * @param task Task to push.
 *
 * @retval 0 Success.
 * @retval != Error code.
 *     - TPOOL_ERR_TOO_MANY_TASKS - pool has too many tasks
 *       already.
 */
int
thread_pool_push_task(struct thread_pool *pool, struct thread_task *task);

/** Thread pool task API. */

/**
 * Create a new task to push it into a pool.
 * @param[out] task Pointer to store result task object.
 * @param function Function to run by this task.
 * @param arg Argument for @a function.
 *
 * @retval Always 0.
 */
int
thread_task_new(struct thread_task **task, thread_task_f function, void *arg);

/**
 * Check if @a task is finished and its result can be obtained.
 * @param task Task to check.
 */
bool
thread_task_is_finished(const struct thread_task *task);

/**
 * Check if @a task is running right now.
 * @param task Task to check.
 */
bool
thread_task_is_running(const struct thread_task *task);

/**
 * Join the task. If it is not finished, then wait until it is.
 * Note, this function does not delete task object. It can be
 * reused for a next task or deleted via thread_task_delete.
 * @param task Task to join.
 * @param[out] result Pointer to stored result of @a task.
 *
 * @retval 0 Success.
 * @retval != 0 Error code.
 *     - TPOOL_ERR_TASK_NOT_PUSHED - task is not pushed to a pool.
 */
int
thread_task_join(struct thread_task *task, void **result);

#ifdef NEED_TIMED_JOIN

/**
 * Like thread_task_join() but wait no longer than the timeout.
 * @param task Task to join.
 * @param timeout Timeout in seconds. 0 means no waiting at all. For an infinite
 *   timeout pass infinity or DBL_MAX or just something huge.
 * @param[out] result Pointer to stored result of @a task.
 *
 * @retval 0 Success.
 * @retval != 0 Error code.
 *     - TPOOL_ERR_TASK_NOT_PUSHED - task is not pushed to a pool.
 *     - TPOOL_ERR_TIMEOUT - join timed out, nothing is done.
 */
int
thread_task_timed_join(struct thread_task *task, double timeout, void **result);

#endif

/**
 * Delete a task, free its memory.
 * @param task Task to delete.
 *
 * @retval 0 Success.
 * @retval != Error code.
 *     - TPOOL_ERR_TASK_IN_POOL - can not drop the task. It still
 *       is in a pool. Need to join it firstly.
 */
int
thread_task_delete(struct thread_task *task);

#ifdef NEED_DETACH

/**
 * Detach a task so as to auto-delete it when it is finished.
 * After detach a task can not be accessed via any functions.
 * If it is already finished, then just delete it.
 * @param task Task to detach.
 * @retval 0 Success.
 * @retval != Error code.
 *     - TPOOL_ERR_TASK_NOT_PUSHED - task is not pushed to a
 *       pool.
*/
int
thread_task_detach(struct thread_task *task);

#endif
