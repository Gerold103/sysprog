#include "thread_pool.h"
#include "unit.h"
#include <pthread.h>
#include <unistd.h>
#include <stdint.h>

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
	__atomic_add_fetch((int *)arg, 1, __ATOMIC_RELAXED);
	return arg;
}

static void *
task_wait_for_f(void *arg)
{
	while(__atomic_load_n((int *)arg, __ATOMIC_RELAXED) == 0)
		usleep(100);
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
	/*
	 * Can delete before push.
	 */
	unit_check(thread_task_new(&t, task_incr_f, &arg) == 0,
		   "created new task");
	unit_check(thread_task_delete(t) == 0,
		   "task can be deleted before push");
	unit_fail_if(thread_task_new(&t, task_incr_f, &arg) != 0);
	/*
	 * Bad join or delete.
	 */
	unit_check(thread_task_join(t, &result) == TPOOL_ERR_TASK_NOT_PUSHED,
		   "can't join a not pushed task");
	unit_check(thread_pool_push_task(p, t) == 0, "pushed");
	unit_check(thread_task_delete(t) == TPOOL_ERR_TASK_IN_POOL,
		   "can't delete before join");
	/*
	 * Normal push.
	 */
	unit_check(thread_task_join(t, &result) == 0, "joined");
	unit_check(result == &arg && arg == 1, "the task really did something");
	unit_check(thread_pool_thread_count(p) == 1, "one active thread");
	/*
	 * Re-push.
	 */
	unit_check(thread_pool_push_task(p, t) == 0, "pushed again");
	unit_check(thread_task_join(t, &result) == 0, "joined");
	unit_check(thread_pool_thread_count(p) == 1, "still one active thread");
	unit_check(thread_task_delete(t) == 0, "deleted after join");
	/*
	 * Re-push stress.
	 */
	const int count = 1000;
	struct thread_task **tasks = malloc(sizeof(*tasks) * count);
	arg = 0;
	for (int i = 0; i < count; ++i) {
		struct thread_task **tp = &tasks[i];
		unit_fail_if(thread_task_new(tp, task_wait_for_f, &arg) != 0);
		unit_fail_if(thread_pool_push_task(p, *tp) != 0);
	}
	__atomic_store_n(&arg, 1, __ATOMIC_RELAXED);
	for (int i = 0; i < count; ++i) {
		t = tasks[i];
		unit_fail_if(thread_task_join(t, &result) != 0);
		unit_fail_if(result != &arg);
		if (i % 2 == 0)
			unit_fail_if(thread_pool_push_task(p, t) != 0);
		else
			unit_fail_if(thread_task_delete(t) != 0);
	}
	for (int i = 0; i < count; i += 2) {
		t = tasks[i];
		unit_fail_if(thread_task_join(t, &result) != 0);
		unit_fail_if(result != &arg);
		unit_fail_if(thread_task_delete(t) != 0);
	}
	free(tasks);
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
	/*
	 * Delete won't work while the pool has tasks.
	 */
	unit_fail_if(thread_pool_new(3, &p) != 0);
	unit_fail_if(thread_task_new(&t, task_lock_unlock_f, &m) != 0);

	pthread_mutex_lock(&m);
	unit_fail_if(thread_pool_push_task(p, t) != 0);
	/* Give the task a chance to be picked up by a thread. */
	usleep(1000);
	unit_check(thread_pool_delete(p) == TPOOL_ERR_HAS_TASKS, "delete does "\
		   "not work until there are not finished tasks");
	pthread_mutex_unlock(&m);
	unit_fail_if(thread_task_join(t, &result) != 0);
	unit_fail_if(thread_task_delete(t) != 0);

	unit_check(thread_pool_delete(p) == 0, "now delete works");
	pthread_mutex_destroy(&m);

	unit_test_finish();
}

static void
map_reduce_inc(struct thread_pool *p, struct thread_task **tasks, int count,
	       int *arg)
{
	void *result;
	for (int i = 0; i < count; ++i) {
		struct thread_task **t = &tasks[i];
		unit_fail_if(thread_task_new(t, task_incr_f, arg) != 0);
		unit_fail_if(thread_pool_push_task(p, *t) != 0);
	}
	for (int i = 0; i < count; ++i) {
		struct thread_task *t = tasks[i];
		unit_fail_if(thread_task_join(t, &result) != 0);
		unit_fail_if(thread_task_delete(t) != 0);
		unit_fail_if(result != arg);
	}
}

static void
test_thread_pool_max_tasks(void)
{
	unit_test_start();

	struct thread_pool *p;
	int more_count = 10;
	int total_count = more_count + TPOOL_MAX_TASKS;
	struct thread_task **tasks = malloc(sizeof(*tasks) * total_count);
	unit_fail_if(thread_pool_new(5, &p) != 0);
	/*
	 * Push max tasks and join all.
	 */
	int arg = 0;
	map_reduce_inc(p, tasks, TPOOL_MAX_TASKS, &arg);
	unit_check(arg == TPOOL_MAX_TASKS, "max tasks are finished");
	arg = 0;
	/*
	 * Push a few more to be sure the pool doesn't overflow any counters
	 * after max.
	 */
	map_reduce_inc(p, tasks, more_count, &arg);
	unit_check(arg == more_count, "a few more tasks are finished");
	/*
	 * Count > max gives an error.
	 */
	arg = 0;
	for (int i = 0; i < TPOOL_MAX_TASKS; ++i) {
		struct thread_task **t = &tasks[i];
		unit_fail_if(thread_task_new(t, task_wait_for_f, &arg) != 0);
		unit_fail_if(thread_pool_push_task(p, *t) != 0);
	}
	/*
	 * Slight overuse is allowed when students do not count tasks being
	 * in-progress. From certain point of view it might be fair.
	 */
	int overuse = 0;
	for (int i = TPOOL_MAX_TASKS; i < total_count; ++i) {
		struct thread_task **t = &tasks[i];
		unit_fail_if(thread_task_new(t, task_wait_for_f, &arg) != 0);
		int rc = thread_pool_push_task(p, *t);
		if (rc == 0) {
			++overuse;
			continue;
		}
		unit_fail_if(thread_task_delete(*t) != 0);
		unit_check(rc == TPOOL_ERR_TOO_MANY_TASKS, "too many tasks");
		break;
	}
	unit_check(overuse < more_count, "reached max tasks");
	__atomic_store_n(&arg, 1, __ATOMIC_RELAXED);
	for (int i = 0; i < TPOOL_MAX_TASKS + overuse; ++i) {
		void *result;
		struct thread_task *t = tasks[i];
		unit_fail_if(thread_task_join(t, &result) != 0);
		unit_fail_if(thread_task_delete(t) != 0);
		unit_fail_if(result != &arg);
	}
	free(tasks);
	unit_fail_if(thread_pool_delete(p) != 0);

	unit_test_finish();
}


static void
test_timed_join(void)
{
#ifdef NEED_TIMED_JOIN
	unit_test_start();

	struct thread_pool *p;
	struct thread_task *task;
	int arg = 0;
	void *result;
	unit_fail_if(thread_pool_new(5, &p) != 0);
	unit_fail_if(thread_task_new(&task, task_wait_for_f, &arg) != 0);
	unit_fail_if(thread_pool_push_task(p, task) != 0);
	unit_check(thread_task_timed_join(task, 0, &result) ==
		TPOOL_ERR_TIMEOUT, "timed out on 0");

	struct timespec ts1, ts2;
	clock_gettime(CLOCK_MONOTONIC, &ts1);
	unit_check(thread_task_timed_join(task, 0.1, &result) ==
		TPOOL_ERR_TIMEOUT, "timeout out on 100 ms");
	clock_gettime(CLOCK_MONOTONIC, &ts2);
	uint64_t ns1 = ts1.tv_sec * 1000000000 + ts1.tv_nsec;
	uint64_t ns2 = ts2.tv_sec * 1000000000 + ts2.tv_nsec;
	unit_check(ns2 - ns1 > 100000000, "didn't exit too early");

	unit_check(thread_task_timed_join(task, -10000, &result) ==
		TPOOL_ERR_TIMEOUT, "timeout out on negative timeout");

	clock_gettime(CLOCK_MONOTONIC, &ts1);
	__atomic_store_n(&arg, 1, __ATOMIC_RELAXED);
	unit_check(thread_task_timed_join(task, 1, &result) == 0,
		"joined with 1 sec timeout");
	clock_gettime(CLOCK_MONOTONIC, &ts2);
	ns1 = ts1.tv_sec * 1000000000 + ts1.tv_nsec;
	ns2 = ts2.tv_sec * 1000000000 + ts2.tv_nsec;
	unit_check(ns2 - ns1 < 1000000000, "1 sec didn't pass");

	unit_fail_if(result != &arg);
	unit_fail_if(thread_task_delete(task) != 0);
	unit_fail_if(thread_pool_delete(p) != 0);

	unit_test_finish();
#endif
}

static void
test_detach_stress(void)
{
#ifdef NEED_DETACH
	unit_test_start();

	struct thread_pool *p;
	int arg = 0;
	struct thread_task *task;
	unit_fail_if(thread_pool_new(5, &p) != 0);
	/*
	 * Push a pack of tasks which should delete themselves. The fact of
	 * deletion needs to be checked with a sanitizer like heap_help.
	 */
	unit_check(true, "detach pushed tasks");
	for (int i = 0; i < 1000; ++i) {
		unit_fail_if(thread_task_new(&task, task_incr_f, &arg) != 0);
		unit_fail_if(thread_pool_push_task(p, task) != 0);
		if (thread_task_detach(task) != 0)
			unit_check(false, "detach failed");
	}
	while (__atomic_load_n(&arg, __ATOMIC_RELAXED) != 1000)
		usleep(1000);
	/*
	 * Non-pushed task can not be detached.
	 */
	unit_fail_if(thread_task_new(&task, task_incr_f, &arg) != 0);
	unit_check(thread_task_detach(task) == TPOOL_ERR_TASK_NOT_PUSHED,
		   "detach non-pushed task");
	unit_fail_if(thread_task_delete(task) != 0);
	// Might be unable to delete the pool right away - the task needs time
	// to complete.
	while (thread_pool_delete(p) != 0)
		usleep(100);

	unit_test_finish();
#endif
}

static void
test_detach_long(void)
{
#ifdef NEED_DETACH
	unit_test_start();

	struct thread_pool *p;
	int arg = 0;
	unit_fail_if(thread_pool_new(5, &p) != 0);
	struct thread_task *task;
	unit_fail_if(thread_task_new(&task, task_wait_for_f, &arg) != 0);
	unit_fail_if(thread_pool_push_task(p, task) != 0);
	// Give it a chance to reach any worker thread.
	usleep(1000);
	unit_check(thread_task_detach(task) == 0, "detach a long task");
	__atomic_store_n(&arg, 1, __ATOMIC_RELAXED);
	// Might be unable to delete the pool right away - the task needs time
	// to complete.
	while (thread_pool_delete(p) != 0)
		usleep(100);

	unit_test_finish();
#endif
}

int
main(void)
{
	unit_test_start();

	test_new();
	test_push();
	test_thread_pool_delete();
	test_thread_pool_max_tasks();
	test_timed_join();
	test_detach_stress();
	test_detach_long();

	unit_test_finish();
	return 0;
}
