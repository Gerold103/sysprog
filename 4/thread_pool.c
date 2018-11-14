#include "thread_pool.h"
#include <pthread.h>

struct thread_task {
	thread_task_f function;
	void *arg;

	/* PUT HERE OTHER MEMBERS */
};

struct thread_pool {
	pthread_t *threads;

	/* PUT HERE OTHER MEMBERS */
};

int
thread_pool_new(int max_thread_count, struct thread_pool **pool)
{
	/* IMPLEMENT THIS FUNCTION */
}

int
thread_pool_thread_count(const struct thread_pool *pool)
{
	/* IMPLEMENT THIS FUNCTION */
}

int
thread_pool_delete(struct thread_pool *pool)
{
	/* IMPLEMENT THIS FUNCTION */
}

int
thread_pool_push_task(struct thread_pool *pool, struct thread_task *task)
{
	/* IMPLEMENT THIS FUNCTION */
}

int
thread_task_new(struct thread_task **task, thread_task_f function, void *arg)
{
	/* IMPLEMENT THIS FUNCTION */
}

bool
thread_task_is_finished(const struct thread_task *task)
{
	/* IMPLEMENT THIS FUNCTION */
}

bool
thread_task_is_running(const struct thread_task *task)
{
	/* IMPLEMENT THIS FUNCTION */
}

int
thread_task_join(struct thread_task *task, void **result)
{
	/* IMPLEMENT THIS FUNCTION */
}

int
thread_task_delete(struct thread_task *task)
{
	/* IMPLEMENT THIS FUNCTION */
}
