#include "thread_pool.h"

#include <pthread.h>

struct thread_task {
	thread_task_f function;

	/* PUT HERE OTHER MEMBERS */
};

struct thread_pool {
	std::vector<pthread_t> threads;

	/* PUT HERE OTHER MEMBERS */
};

int
thread_pool_new(int thread_count, struct thread_pool **pool)
{
	/* IMPLEMENT THIS FUNCTION */
	(void)thread_count;
	(void)pool;
	return TPOOL_ERR_NOT_IMPLEMENTED;
}

int
thread_pool_delete(struct thread_pool *pool)
{
	/* IMPLEMENT THIS FUNCTION */
	(void)pool;
	return TPOOL_ERR_NOT_IMPLEMENTED;
}

int
thread_pool_push_task(struct thread_pool *pool, struct thread_task *task)
{
	/* IMPLEMENT THIS FUNCTION */
	(void)pool;
	(void)task;
	return TPOOL_ERR_NOT_IMPLEMENTED;
}

int
thread_task_new(struct thread_task **task, const thread_task_f &function)
{
	/* IMPLEMENT THIS FUNCTION */
	(void)task;
	(void)function;
	return TPOOL_ERR_NOT_IMPLEMENTED;
}

bool
thread_task_is_finished(const struct thread_task *task)
{
	/* IMPLEMENT THIS FUNCTION */
	(void)task;
	return false;
}

bool
thread_task_is_running(const struct thread_task *task)
{
	/* IMPLEMENT THIS FUNCTION */
	(void)task;
	return false;
}

int
thread_task_join(struct thread_task *task)
{
	/* IMPLEMENT THIS FUNCTION */
	(void)task;
	return TPOOL_ERR_NOT_IMPLEMENTED;
}

#if NEED_TIMED_JOIN

int
thread_task_timed_join(struct thread_task *task, double timeout)
{
	/* IMPLEMENT THIS FUNCTION */
	(void)task;
	(void)timeout;
	return TPOOL_ERR_NOT_IMPLEMENTED;
}

#endif

int
thread_task_delete(struct thread_task *task)
{
	/* IMPLEMENT THIS FUNCTION */
	(void)task;
	return TPOOL_ERR_NOT_IMPLEMENTED;
}

#if NEED_DETACH

int
thread_task_detach(struct thread_task *task)
{
	/* IMPLEMENT THIS FUNCTION */
	(void)task;
	return TPOOL_ERR_NOT_IMPLEMENTED;
}

#endif
