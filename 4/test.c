#include "thread_pool.h"
#include "unit.h"
#include <pthread.h>

static void
test_new(void)
{
	unit_test_start();

	struct thread_pool *p;
	unit_check(thread_pool_new(TPOOL_MAX_THREADS + 1, &p) ==
		   TPOOL_ERR_INVALID_ARGUMENT, "too big thread count is "\
		   "forbidden");
	unit_check(thread_pool_new(0, &p) == TPOOL_ERR_INVALID_ARGUMENT,
		   "0 thread count is forbidden");
	unit_check(thread_pool_new(-1, &p) == TPOOL_ERR_INVALID_ARGUMENT,
		   "negative thread count is forbidden");

	unit_check(thread_pool_new(1, &p) == 0, "1 max thread is allowed");
	unit_check(thread_pool_thread_count(p) == 0,
		   "0 active threads after creation");
	unit_check(thread_pool_delete(p) == 0, "delete without tasks");

	unit_check(thread_pool_new(TPOOL_MAX_THREADS, &p) == 0,
		   "max thread count is allowed");
	unit_check(thread_pool_thread_count(p) == 0,
		   "0 active threads after creation");
	unit_check(thread_pool_delete(p) == 0, "delete");

	unit_test_finish();
}

static void *
task_incr_f(void *arg)
{
	++*((int *) arg);
	return arg;
}

static void
test_push(void)
{
	unit_test_start();

	struct thread_pool *p;
	struct thread_task *t;
	unit_fail_if(thread_pool_new(3, &p) != 0);
	int arg = 0;
	void *result;
	unit_check(thread_task_new(&t, task_incr_f, &arg) == 0,
		   "created new task");
	unit_check(thread_task_delete(t) == 0,
		   "task can be deleted before push");
	unit_fail_if(thread_task_new(&t, task_incr_f, &arg) != 0);

	unit_check(thread_task_join(t, &result) == TPOOL_ERR_TASK_NOT_PUSHED,
		   "can't join a not pushed task");
	unit_check(thread_pool_push_task(p, t) == 0, "pushed");
	unit_check(thread_task_delete(t) == TPOOL_ERR_TASK_IN_POOL,
		   "can't delete before join");
	unit_check(thread_task_join(t, &result) == 0, "joined");
	unit_check(result == &arg && arg == 1, "the task really did something");

	unit_check(thread_pool_thread_count(p) == 1, "one active thread");
	unit_check(thread_pool_push_task(p, t) == 0, "pushed again");
	unit_check(thread_task_join(t, &result) == 0, "joined");
	unit_check(thread_pool_thread_count(p) == 1, "still one active thread");
	unit_check(thread_task_delete(t) == 0, "deleted after join");

	unit_fail_if(thread_pool_delete(p) != 0);

	unit_test_finish();
}

static void *
task_lock_unlock_f(void *arg)
{
	pthread_mutex_t *m = (pthread_mutex_t *) arg;
	pthread_mutex_lock(m);
	pthread_mutex_unlock(m);
	return arg;
}

static void
test_thread_pool_delete(void)
{
	unit_test_start();

	void *result;
	struct thread_pool *p;
	struct thread_task *t;
	pthread_mutex_t m;
	pthread_mutex_init(&m, NULL);
	unit_fail_if(thread_pool_new(3, &p) != 0);
	unit_fail_if(thread_task_new(&t, task_lock_unlock_f, &m) != 0);

	pthread_mutex_lock(&m);
	unit_fail_if(thread_pool_push_task(p, t) != 0);
	unit_check(thread_pool_delete(p) == TPOOL_ERR_HAS_TASKS, "delete does "\
		   "not work until there are not finished tasks");
	pthread_mutex_unlock(&m);
	unit_fail_if(thread_task_join(t, &result) != 0);
	unit_fail_if(thread_task_delete(t) != 0);

	unit_check(thread_pool_delete(p) == 0, "now delete works");
	pthread_mutex_destroy(&m);

	unit_test_finish();
}

int
main(void)
{
	unit_test_start();

	test_new();
	test_push();
	test_thread_pool_delete();

	unit_test_finish();
	return 0;
}
